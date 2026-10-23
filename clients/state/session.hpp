#ifndef SESSION_HPP
# define SESSION_HPP

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include <deque>
# include <functional>
# include <string>

# include "../client.hpp"
# include "catalog.hpp"
# include "game_state.hpp"
# include "message.hpp"

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

// The part of the state that changed: the GUI redraws only that part.
enum class Section {
	Connection,
	Room,
	Inventory,
	Status,
	Combat,
	Counters,
	Chat,
	Group,
	Log
};

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

/**
 * The client's brain.
 *
 * It sends commands to the server and remembers them in a FIFO queue; every
 * OK or ERR from the server is routed to the handler of the command at the
 * head of the queue, because the server answers every command with exactly
 * one line, in the order of arrival. Events (EVT) never touch the queue.
 *
 * The Session owns the GameState and is the only one allowed to write into
 * it. It uses the Client only to send: the connection is managed elsewhere.
 */
class Session
{
	private:
		// the only thing we use from Client: send one line
		Client	&_client;

		// the commands whose answer is awaited, in sending order
		struct Pending {
			std::string	command;	// "TAKE"
			std::string	args;		// "item.herbs#3", empty if none
		};
		std::deque<Pending>	_pending;

		// what the GUI reads (through state())
		GameState	_state;
		Catalog		_catalog;

		// called on every change, with the part that changed
		std::function<void(Section)>	_on_changed;

		// Log a technical line in the journal, and announce it to the GUI.
		void	log(const std::string& line);

		// Announce one section to the GUI, if it registered a callback.
		void	changed(Section section);

		// Route one answer to the handler of the command it belongs to.
		void	handleOk(const Pending& pending, const ServerMessage& message);
		void	handleErr(const Pending& pending, const ServerMessage& message);

		// Apply one answer to the state, then warn the GUI.
		void	applyLook(const std::string& data);
		void	applyInventory(const std::string& data);
		void	applyStatus(const std::string& data);
		void	applyWho(const std::string& data);
		void	applyMove(const std::string& data);
		void	applyTake(const std::string& data);
		void	applyDrop(const std::string& data);
		void	applyTalk(const std::string& data);

		// ATTACK, DEFEND and FLEE share one answer format: one handler for all three
		void	applyCombat(const std::string& command, const std::string& data);

		// GROUP CREATE / JOIN / LEAVE: args says which one we asked
		void	applyGroup(const std::string& args, const std::string& data);

		// Route one event. Events never consume the queue.
		void	handleEvent(const ServerMessage& message);

		// EVT ROOM ...: the data starts with a sub-type (PRESENCE, COMBAT, SPAWN)
		void	handleRoomEvent(const std::string& data);

		// EVT ROOM PRESENCE ENTER <player> / LEAVE <player>
		void	applyPresence(const std::string& action, const std::string& player);

		// EVT ROOM COMBAT ...: one sub-type per event, all sharing <player> <enemy>
		void	applyCombatEvent(const std::vector<std::string>& words);

		// "CHAT <author> <message...>": the message may hold spaces, so only
		// the author is cut out; the whole rest is the text.
		void	applyChat(std::vector<ChatLine>& history, const std::string& data);

		// EVT GROUP INVITE <leader> / JOIN <player> / LEAVE <player>
		void	applyGroupEvent(const std::string& data);

	public:
		// The greeting arrives before any command: the queue starts with
		// HELLO so that the first OK has a head to belong to.
		explicit Session(Client &client);

		// Send a command to the server, and remember it in the queue.
		// args may be empty: send("LOOK") sends exactly "LOOK".
		void	send(const std::string& command, const std::string& args = "");

		// Take the three actions of the Client ("OK", "ERR", "EVT").
		// Client keeps ONE handler per type: whoever calls this last owns them,
		// so nobody else must call defineAction on these three types.
		void	listen();

		// Feed one line from the server, without its line end.
		void	handleLine(const std::string& line);

		// Client::update() returned false: the socket is gone.
		void	handleDisconnect();

		// A new connection is about to start: forget the previous game.
		// The HELLO greeting opens the queue again, as at construction.
		void	restart();

		// Called once per change, with the part that changed.
		void	onChanged(std::function<void(Section)> callback);

		// The GUI reads this; only the Session writes into it.
		const GameState&	state() const;

		// The GUI reads this too, to turn the ids of the state into names.
		// Read only: the catalog is filled once, at construction.
		const Catalog&	catalog() const;
};

#endif