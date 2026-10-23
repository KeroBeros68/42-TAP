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

		// Route one event. Events never consume the queue.
		void	handleEvent(const ServerMessage& message);

	public:
		// The greeting arrives before any command: the queue starts with
		// HELLO so that the first OK has a head to belong to.
		explicit Session(Client &client);

		// Send a command to the server, and remember it in the queue.
		// args may be empty: send("LOOK") sends exactly "LOOK".
		void	send(const std::string& command, const std::string& args = "");

		// Feed one line from the server, without its line end.
		void	handleLine(const std::string& line);

		// Called once per change, with the part that changed.
		void	onChanged(std::function<void(Section)> callback);

		// The GUI reads this; only the Session writes into it.
		const GameState&	state() const;
};

#endif