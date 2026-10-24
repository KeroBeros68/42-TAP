#include "session.hpp"

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../shared/protocol/event.hpp"	// EVT_ROOM, EVT_STATS
# include <cstdlib>					// std::atoi

//##############################################################################
//#                             INTERNAL CONSTANTS                             #
//##############################################################################

using json = nlohmann::json;

namespace {

// The journal keeps the latest lines only: a long session must not grow forever
const size_t	LOG_MAX_LINES = 500;

// Sub-type of "EVT ROOM ...". There is no constant for it in event.hpp:
// EVT_PRESENCE_ENTER is the whole sub-type and action ("PRESENCE ENTER").
const char* const	EVT_PRESENCE = "PRESENCE";

// The chat scopes (GLOBAL, ROOM, GROUP) and the chat type (CHAT) are named by
// event.hpp: EVT_GLOBAL, EVT_GROUP and EVT_CHAT.

bool	contains(const std::vector<std::string>& list, const std::string& value)
{
	for (const std::string& element : list)
		if (element == value)
			return true;
	return false;
}

// Takes the first word out of text, and leaves the rest in rest.
// Same job as takeFirstWord() in message.cpp, which is private to that file.
std::string	takeWord(const std::string& text, std::string& rest)
{
	size_t begin = 0;
	while (begin < text.size() && text[begin] == CMD_SEPARATOR)
		++begin;
	size_t end = begin;
	while (end < text.size() && text[end] != CMD_SEPARATOR)
		++end;
	const std::string word = text.substr(begin, end - begin);
	while (end < text.size() && text[end] == CMD_SEPARATOR)
		++end;
	rest = text.substr(end);
	return word;
}

// Cuts text on its separators: "START alice npc.goblin" -> {"START", "alice", "npc.goblin"}
std::vector<std::string>	splitWords(const std::string& text)
{
	std::vector<std::string>	words;
	size_t						begin = 0;

	while (begin < text.size())
	{
		while (begin < text.size() && text[begin] == CMD_SEPARATOR)
			++begin;
		size_t end = begin;
		while (end < text.size() && text[end] != CMD_SEPARATOR)
			++end;
		if (end > begin)
			words.push_back(text.substr(begin, end - begin));
		begin = end;
	}
	return words;
}

void	removeFrom(std::vector<std::string>& list, const std::string& value)
{
	for (std::vector<std::string>::iterator it = list.begin(); it != list.end(); ++it)
		if (*it == value)
		{
			list.erase(it);
			return;
		}
}

// One line of journal for one turn of a fight.
// A turn looks like: {actor, target, round, action, missed, damage}.
// Everything is read with a default: a server leaving a field out must not
// break the display (interchangeability rule).
std::string	describeTurn(const nlohmann::json& turn)
{
	if (!turn.is_object())
		return "combat: unreadable turn";

	const std::string	actor = turn.value(FIELD_ACTOR, "?");
	const std::string	target = turn.value(FIELD_TARGET, "?");
	const std::string	action = turn.value(FIELD_ACTION, "?");
	const int			round = turn.value(FIELD_ROUND, 0);
	const bool			missed = turn.value(FIELD_MISSED, false);
	const int			damage = turn.value(FIELD_DAMAGE, 0);

	std::string	line = "round " + std::to_string(round) + ": " + actor + " ";
	if (action == ACTION_ATTACK)
		line += missed ? "misses " + target : "hits " + target + " for "
			+ std::to_string(damage) + " damage";
	else if (action == ACTION_DEFEND)
		line += "defends";
	else if (action == ACTION_FLEE)
		line += missed ? "fails to flee" : "flees";
	else
		line += action + " " + target;
	return line;
}

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

	// A new attempt: the previous failure is no longer news
	_state.last_error.reset();
}


void	Session::listen()
{
	// Client::handleLine splits the line and gives us only the arguments:
	// "OK {…}" arrives here as "{…}". Put the kind back in front, so the
	// same parser handles lines from the socket and lines injected in tests.
	_client.defineAction("OK", [this](const std::string& args) {
		this->handleLine(std::string(RESPONSE_OK) + CMD_SEPARATOR + args);
	});
	_client.defineAction("ERR", [this](const std::string& args) {
		this->handleLine(std::string(RESPONSE_ERR) + CMD_SEPARATOR + args);
	});
	_client.defineAction("EVT", [this](const std::string& args) {
		this->handleLine(std::string(RESPONSE_EVT) + CMD_SEPARATOR + args);
	});
}

void	Session::restart()
{
	// Everything the previous server told us is now wrong
	_pending.clear();
	_state = GameState();
	_pending.push_back({"HELLO", ""});
	changed(Section::Connection);
}

void	Session::handleDisconnect()
{
	// We no longer have a queue: the answers to those commands will never come
	_pending.clear();
	_state.phase = GameState::Phase::TERMINATED;
	log("connection lost");
	changed(Section::Connection);
}

void	Session::onChanged(std::function<void(Section)> callback)
{
	_on_changed = callback;
}

const GameState&	Session::state() const
{
	return _state;
}

const Catalog&	Session::catalog() const
{
	return _catalog;
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

	if (pending.command == "MOVE")
	{
		applyMove(message.data);
		return;
	}

	if (pending.command == "TAKE")
	{
		applyTake(message.data);
		return;
	}

	if (pending.command == "DROP")
	{
		applyDrop(message.data);
		return;
	}

	if (pending.command == "TALK")
	{
		applyTalk(message.data);
		return;
	}

	if (pending.command == "ATTACK" || pending.command == "DEFEND"
		|| pending.command == "FLEE")
	{
		applyCombat(pending.command, message.data);
		return;
	}

	if (pending.command == "CHAT")
	{
		// Nothing to apply: the server sends the message back to its author
		// as an EVT, and that is where the chat is filled. Applying it here
		// too would display it twice.
		return;
	}

	if (pending.command == "GROUP")
	{
		applyGroup(pending.args, message.data);
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

void	Session::applyMove(const std::string& data)
{
	// "OK room=<id>": the room id only. We moved, so the old room content is
	// wrong now: ask for a fresh LOOK instead of guessing.
	const size_t separator = data.find('=');
	if (separator == std::string::npos)
	{
		log("MOVE: unreadable answer: " + data);
		return;
	}
	_state.room.id = data.substr(separator + 1);
	_state.room.name.clear();
	_state.room.description.clear();
	_state.room.exits.clear();
	_state.players.clear();
	_state.items.clear();
	_state.npcs.clear();

	changed(Section::Room);
	send("LOOK");
}

void	Session::applyTake(const std::string& data)
{
	// "OK taken=<id>": move the id from the floor to the inventory.
	// The local move is optimistic: the LOOK and INVENTORY that follow
	// correct it if another player was faster.
	const size_t separator = data.find('=');
	if (separator == std::string::npos)
	{
		log("TAKE: unreadable answer: " + data);
		return;
	}
	const std::string id = data.substr(separator + 1);

	removeFrom(_state.items, id);
	if (!contains(_state.inventory, id))
		_state.inventory.push_back(id);

	changed(Section::Room);
	changed(Section::Inventory);

	// The server is the source of truth: resynchronize both lists
	send("LOOK");
	send("INVENTORY");
}

void	Session::applyCombat(const std::string& command, const std::string& data)
{
	const std::optional<json> payload = asJson(data);
	if (!payload || !payload->is_object())
	{
		log(command + ": unreadable answer: " + data);
		return;
	}

	const json& report = *payload;

	// Our hit points after the turns, told by the server: never computed here
	_state.hp = report.value(FIELD_ATTACKER_HP, _state.hp);
	_state.status = report.value(FIELD_STATUS, _state.status);
	_state.combat.active = true;
	_state.combat.round = report.value(FIELD_ROUND, _state.combat.round);

	// One line per turn in the journal, so the fight can be followed
	if (report.contains(FIELD_TURNS) && report[FIELD_TURNS].is_array())
		for (const auto& turn : report[FIELD_TURNS])
			log(describeTurn(turn));

	const bool escaped = report.value(FIELD_ESCAPED, false);
	if (escaped)
	{
		// We ran away: the room changed, its content must be asked again
		_state.combat.active = false;
		_state.room.name.clear();
		_state.room.description.clear();
		_state.room.exits.clear();
		_state.players.clear();
		_state.items.clear();
		_state.npcs.clear();
		changed(Section::Room);
		send("LOOK");
	}
	else
	{
		// Fresh hit points, status and turn information
		send("STATUS");
	}

	changed(Section::Combat);
	changed(Section::Status);
}

void	Session::applyGroup(const std::string& args, const std::string& data)
{
	// args is the sub-command: "CREATE", "JOIN <leader>" or "LEAVE".
	// The first word tells which one we asked.
	const std::string	sub = args.substr(0, args.find(' '));

	if (sub.empty())
	{
		// "GROUP" alone is not a command the server understands
		log("GROUP: missing sub-command");
		return;
	}

	if (sub == "LEAVE")
	{
		// "OK": we left, we belong to nobody any more
		_state.group.reset();
		log("left the group");
		changed(Section::Group);
		return;
	}

	// CREATE and JOIN both answer "OK group=<id>"
	const size_t separator = data.find('=');
	if (separator == std::string::npos)
	{
		log("GROUP " + sub + ": unreadable answer: " + data);
		return;
	}
	_state.group = data.substr(separator + 1);
	_state.pending_invite.reset();	// the invitation is accepted or refused

	// Read it back rather than dereference: a server answering "OK group="
	// would give an empty id.
	log("group " + sub + ": " + _state.group.value_or(""));
	changed(Section::Group);
}

void	Session::applyTalk(const std::string& data)
{
	// Our server answers "OK <dialogue>"; the subject shows that some servers
	// answer "OK {"npc":...,"dialogue":...}". Accept both.
	std::string said = data;
	const std::optional<json> payload = asJson(data);
	if (payload && payload->is_object() && payload->contains("dialogue"))
		said = payload->value("dialogue", data);

	if (said.empty())
	{
		log("TALK: empty answer");
		return;
	}
	log("TALK: " + said);
}

void	Session::applyDrop(const std::string& data)
{
	// "OK dropped=<id>": the mirror of TAKE
	const size_t separator = data.find('=');
	if (separator == std::string::npos)
	{
		log("DROP: unreadable answer: " + data);
		return;
	}
	const std::string id = data.substr(separator + 1);

	removeFrom(_state.inventory, id);
	if (!contains(_state.items, id))
		_state.items.push_back(id);

	changed(Section::Room);
	changed(Section::Inventory);

	send("LOOK");
	send("INVENTORY");
}

void	Session::handleErr(const Pending& pending, const ServerMessage& message)
{
	// The code and the name are both kept: 404 means three different things
	_state.last_error = std::to_string(message.code) + " " + message.name;
	log(pending.command + " -> ERR " + *_state.last_error);
	changed(Section::Connection);

	// A TAKE that failed means another player probably took the item first:
	// our list of the floor is stale, ask the server again.
	// The name is checked, not the code: 404 is shared by three errors.
	if (pending.command == "TAKE" && message.name == "ITEM_NOT_FOUND")
		send("LOOK");
}

void	Session::handleEvent(const ServerMessage& message)
{
	// The category is the big family: ROOM, STATS, GLOBAL, GROUP...
	if (message.category == EVT_STATS)
	{
		// "EVT STATS players=42": the whole server count, nothing else
		const size_t separator = message.data.find('=');
		if (separator != std::string::npos)
		{
			_state.server_players = std::atoi(message.data.c_str() + separator + 1);
			changed(Section::Counters);
			return;
		}
	}
	else if (message.category == EVT_ROOM)
	{
		handleRoomEvent(message.data);
		return;
	}
	else if (message.category == EVT_GLOBAL)
	{
		// "EVT GLOBAL CHAT <author> <message>"
		applyChat(_state.chat_global, message.data);
		return;
	}
	else if (message.category == EVT_GROUP)
	{
		// "EVT GROUP CHAT <author> <message>", INVITE, JOIN or LEAVE
		std::string rest;
		const std::string sub = takeWord(message.data, rest);
		if (sub == EVT_CHAT)
			applyChat(_state.chat_group, message.data);
		else
			applyGroupEvent(message.data);
		return;
	}

	// Everything not handled yet still goes to the journal: the rule of
	// interoperability says an unknown event is never an error.
	log("EVT " + message.category + " " + message.data);
}

void	Session::handleRoomEvent(const std::string& data)
{
	// "PRESENCE ENTER alice": sub-type first, then the rest
	std::string rest;
	const std::string sub = takeWord(data, rest);

	if (sub == EVT_PRESENCE)
	{
		std::string player;
		const std::string action = takeWord(rest, player);
		applyPresence(action, player);
		return;
	}

	if (sub == EVT_COMBAT)
	{
		// "COMBAT START alice npc.goblin": every combat event starts with
		// the sub-type, a player and an enemy, so we split the whole rest.
		applyCombatEvent(splitWords(data));
		return;
	}

	if (sub == EVT_CHAT)
	{
		// "EVT ROOM CHAT <author> <message>": room scope
		applyChat(_state.chat_room, data);
		return;
	}

	if (sub == EVT_SPAWN)
	{
		// "SPAWN npc npc.goblin": something came back into the room.
		// We do not guess where: ask for the room again.
		log("spawn: " + rest);
		send("LOOK");
		return;
	}

	log("EVT ROOM " + data);
}

void	Session::applyGroupEvent(const std::string& data)
{
	// data is "INVITE <leader>", "JOIN <player>" or "LEAVE <player>"
	std::string rest;
	const std::string sub = takeWord(data, rest);

	if (sub == "INVITE")
	{
		// "<leader> invites us": remembered until GROUP JOIN or CREATE answers
		if (rest.empty())
		{
			log("EVT GROUP INVITE: no leader name");
			return;
		}
		_state.pending_invite = rest;
		log(rest + " invites you to a group");
		changed(Section::Group);
		return;
	}

	if (sub == "JOIN")
	{
		log(rest + " joined the group");
		return;
	}

	if (sub == "LEAVE")
	{
		// A named player leaving is not us: GROUP LEAVE is answered by
		// an OK, and that is what clears our own group.
		if (!rest.empty())
		{
			log(rest + " left the group");
			return;
		}
	}

	log("EVT GROUP " + data);
}

void	Session::applyChat(std::vector<ChatLine>& history, const std::string& data)
{
	// data is "CHAT <author> <message...>": cut twice, never more,
	// because the message itself may hold separators.
	std::string rest;
	const std::string kind = takeWord(data, rest);
	if (kind != EVT_CHAT)
	{
		log("chat: unknown event " + data);
		return;
	}

	std::string text;
	const std::string author = takeWord(rest, text);
	if (author.empty() || text.empty())
	{
		log("chat: malformed event " + data);
		return;
	}

	// Chat never goes into the journal: the subject requires a separate view
	history.push_back({author, text});
	if (history.size() > LOG_MAX_LINES)
		history.erase(history.begin());

	changed(Section::Chat);
}

void	Session::applyCombatEvent(const std::vector<std::string>& words)
{
	// words: {"COMBAT", <type>, <player>, <enemy>, ...}
	if (words.size() < 4)
	{
		log("EVT ROOM COMBAT: malformed event");
		return;
	}

	const std::string	&type = words[1];
	const std::string	&player = words[2];
	const std::string	&enemy_id = words[3];
	const std::string	enemy = _catalog.name(enemy_id);
	const bool			mine = (player == _state.me);

	if (type == EVT_COMBAT_START || type == EVT_COMBAT_JOIN)
	{
		log(player + (type == EVT_COMBAT_START ? " starts fighting " : " joins the fight against ") + enemy);
		// Our own fight is the only one this state describes
		if (mine)
		{
			_state.combat.active = true;
			_state.combat.enemy = enemy_id;
			_state.combat.round = 0;
			_state.combat.my_turn = false;
			_state.combat.seconds_left = 0;
			changed(Section::Combat);
		}
		return;
	}

	if (type == EVT_COMBAT_ROUND)
	{
		// "ROUND <actor> <enemy> <round> <action> <target> <result>"
		if (words.size() < 8)
		{
			log("EVT ROOM COMBAT ROUND: malformed event");
			return;
		}
		const std::string	&round = words[4];
		const std::string	&action = words[5];
		const std::string	&target = words[6];
		const std::string	&result = words[7];

		log("round " + round + ": " + player + " " + action + " on " + target
			+ " (" + result + ")");

		// The round number is the server's: it may describe somebody else's
		// fight in the same room, so only our own enemy updates our state.
		if (_state.combat.active && enemy_id == _state.combat.enemy)
			_state.combat.round = std::atoi(round.c_str());
		changed(Section::Combat);
		return;
	}

	if (type == EVT_COMBAT_TURN)
	{
		// "TURN <player> <enemy> <seconds>"
		_state.combat.my_turn = mine;
		if (mine)
		{
			_state.combat.enemy = enemy_id;
			_state.combat.seconds_left = (words.size() > 4) ? std::atoi(words[4].c_str()) : 0;
		}
		log(mine ? "your turn" : player + "'s turn");
		changed(Section::Combat);
		return;
	}

	if (type == EVT_COMBAT_TIMEOUT)
	{
		log(player + " was too slow");
		if (mine)
			_state.combat.my_turn = false;
		changed(Section::Combat);
		return;
	}

	if (type == EVT_COMBAT_END)
	{
		// "END <player> <enemy> <victory|defeat|escaped>"
		const std::string	result = (words.size() > 4) ? words[4] : "over";
		log(player + " leaves the fight against " + enemy + ": " + result);

		if (mine)
		{
			_state.combat = GameState::Combat();	// no fight any more
			// Victory and defeat move nobody; an escape changes the room,
			// but the ATTACK/FLEE answer already asked for a LOOK.
			changed(Section::Combat);
		}
		return;
	}

	log("EVT ROOM COMBAT " + type + ": unknown sub-type");
}

void	Session::applyPresence(const std::string& action, const std::string& player)
{
	if (player.empty())
	{
		log("EVT ROOM PRESENCE: no player name");
		return;
	}

	if (action == "ENTER")
	{
		// No duplicate: the same ENTER can arrive twice (reconnect, LOOK race)
		if (!contains(_state.players, player))
			_state.players.push_back(player);
		changed(Section::Room);
		changed(Section::Counters);
		return;
	}

	if (action == "LEAVE")
	{
		removeFrom(_state.players, player);
		changed(Section::Room);
		changed(Section::Counters);
		return;
	}

	log("EVT ROOM PRESENCE " + action + " " + player);
}
