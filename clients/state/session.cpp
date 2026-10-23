#include "session.hpp"

//##############################################################################
//#                             INTERNAL CONSTANTS                             #
//##############################################################################

using json = nlohmann::json;

namespace {

// The journal keeps the latest lines only: a long session must not grow forever
const size_t	LOG_MAX_LINES = 500;

}

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

// Init session
Session::Session(Client &client)
	: _client(client)
{
	// The greeting arrives before any command, without one of ours:
	// the queue starts with HELLO so the first OK has a head to belong to
	_pending.push_back({"HELLO", ""});  // Dummy command due to OK hello proto=1 needing a corresponding command
}

// Send a command and the args to the Client
void	Session::send(const std::string& command, const std::string& args)
{
	const std::string	line = args.empty() ? command : command + " " + args;
	_client.send(line);
	_pending.push_back({command, args});  // Add the command to the registry
}


void	Session::onChanged(std::function<void(Section)> callback)
{
	_on_changed = callback;
}

const GameState&	Session::state() const
{
	return _state;
}

// Save a given information in the session's logs
void	Session::log(const std::string& line)
{
	_state.log.push_back(line);
	while (_state.log.size() > LOG_MAX_LINES)
		_state.log.pop_front();
	changed(Section::Log);
}

// Call the registered function with the given Section to update
void	Session::changed(Section section)
{
	if (_on_changed)
		_on_changed(section);
}

// Scan the server's response to handle it
void	Session::handleLine(const std::string& line)
{
	const ServerMessage	message = parseLine(line);

	switch (message.kind)
	{
		// OK and ERR are on base on the same usage. We treat both the same 
		// way and dispatch them at the end
		case ServerMessage::Kind::OK:
		case ServerMessage::Kind::ERR:
			// The server answers every command with exactly one OK or ERR,
			// in the order of arrival: the head of the queue is the command
			// this answer belongs to
			if (_pending.empty())
			{
				log("answer without a pending command: " + line);
				return;
			}
			{
				const Pending	pending = _pending.front();
				_pending.pop_front();
				if (message.kind == ServerMessage::Kind::OK)
					handleOk(pending, message);
				else
					handleErr(pending, message);
			}
			break;

		case ServerMessage::Kind::EVT:
			// An event answers no command: it never touches the queue
			handleEvent(message);
			break;

		default:
			log("unexpected line: " + line);
			break;
	}
}

void	Session::handleOk(const Pending& pending, const ServerMessage& message)
{
	if (pending.command == "HELLO")
	{
		// The greeting: the socket is open, but we have not said who we are
		_state.phase = GameState::Phase::CONNECTED;
		log("server greeting: " + message.data);
		changed(Section::Connection);
		return;
	}

	if (pending.command == "CONNECT")
	{
		// The name was accepted: from now on we play.
		_state.phase = GameState::Phase::AUTHENTICATED;
		_state.me = pending.args;	// the name we asked for (the server echoes no name)
		log("authenticated as " + _state.me);
		changed(Section::Connection);

		// Everything the game window needs, asked in one go.
		// Four commands, so four answers, so four entries in the queue.
		send("LOOK");
		send("INVENTORY");
		send("STATUS");
		send("WHO");
		return;
	}

	if (pending.command == "LOOK")
	{
		applyLook(message.data);
		return;
	}

	if (pending.command == "INVENTORY")
	{
		applyInventory(message.data);
		return;
	}

	if (pending.command == "STATUS")
	{
		applyStatus(message.data);
		return;
	}

	if (pending.command == "WHO")
	{
		applyWho(message.data);
		return;
	}

	// Not handled yet: keep the answer visible in the journal
	log(pending.command + " -> OK " + message.data);
}

void	Session::applyLook(const std::string& data)
{
	// Parse json
	const std::optional<json> payload = asJson(data);
	if (!payload || !payload->is_object())
	{
		log("LOOK: unreadable answer: " + data);
		return;
	}

	const json& room = *payload;

	// Update game state
	if (room.contains(FIELD_ROOM) && room[FIELD_ROOM].is_object())
	{
		const json& info = room[FIELD_ROOM];
		_state.room.id = info.value(FIELD_ID, _state.room.id);
		_state.room.name = info.value(FIELD_NAME, _state.room.name);
		_state.room.description = info.value(FIELD_DESCRIPTION, _state.room.description);
		_state.room.exits.clear();
		if (info.contains(FIELD_EXITS) && info[FIELD_EXITS].is_object())
			for (const auto& [direction, target] : info[FIELD_EXITS].items())
				_state.room.exits[direction] = target.get<std::string>();
	}

	// Turn the data to a list of (key) and return it
	const auto readList = [](const json& node, const char* key) {
		std::vector<std::string> result;
		if (node.contains(key) && node[key].is_array())
			for (const auto& value : node[key])
				if (value.is_string())
					result.push_back(value.get<std::string>());
		return result;
	};

	// Update the lists
	_state.players = readList(room, FIELD_PLAYERS);
	_state.items = readList(room, FIELD_ITEMS);
	_state.npcs = readList(room, FIELD_NPCS);

	changed(Section::Room);
	changed(Section::Counters);	// players.size() is the room counter
}

void	Session::applyInventory(const std::string& data)
{
	// 
	const std::optional<json> payload = asJson(data);
	if (!payload || !payload->is_array())
	{
		log("INVENTORY: unreadable answer: " + data);
		return;
	}

	_state.inventory.clear();
	for (const auto& value : *payload)
		if (value.is_string())
			_state.inventory.push_back(value.get<std::string>());

	changed(Section::Inventory);
}

void	Session::applyStatus(const std::string& data)
{
	const std::optional<json> payload = asJson(data);
	if (!payload || !payload->is_object())
	{
		log("STATUS: unreadable answer: " + data);
		return;
	}

	const json& status = *payload;

	_state.hp = status.value(FIELD_HP, _state.hp);
	_state.max_hp = status.value(FIELD_MAX_HP, _state.max_hp);
	_state.status = status.value(FIELD_STATUS, _state.status);

	// The combat fields only come when we are in a fight (our extension)
	if (status.contains(FIELD_ENEMY))
	{
		_state.combat.active = true;
		_state.combat.enemy = status.value(FIELD_ENEMY, _state.combat.enemy);
		_state.combat.round = status.value(FIELD_ROUND, _state.combat.round);
		_state.combat.my_turn = status.value(FIELD_YOUR_TURN, _state.combat.my_turn);
		_state.combat.seconds_left = status.value(FIELD_TURN_SECONDS_LEFT, _state.combat.seconds_left);
	}
	else
		_state.combat.active = false;

	changed(Section::Status);
	changed(Section::Combat);
}

void	Session::applyWho(const std::string& data)
{
	// "OK players=<n>" on our server, "OK {"room":[...],"server":<n>}" elsewhere
	const size_t separator = data.find('=');
	if (separator != std::string::npos
		&& data.compare(0, separator, FIELD_PLAYERS) == 0)
	{
		_state.server_players = std::atoi(data.c_str() + separator + 1);
	}
	else
	{
		const std::optional<json> payload = asJson(data);
		if (!payload || !payload->is_object() || !payload->contains("server"))
		{
			log("WHO: unreadable answer: " + data);
			return;
		}
		_state.server_players = (*payload)["server"].get<int>();
	}

	changed(Section::Counters);
}

void	Session::handleErr(const Pending& pending, const ServerMessage& message)
{
	// The code and the name are both logged: 404 means three different things
	log(pending.command + " -> ERR " + std::to_string(message.code) + " " + message.name);
}

void	Session::handleEvent(const ServerMessage& message)
{
	log("EVT " + message.category + " " + message.data);
}
