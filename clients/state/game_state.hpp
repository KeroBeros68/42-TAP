/*
	The GameState is a data structure representing the State of the Game
	(Current room, connection state, players, etc.).
*/

#ifndef GAME_STATE_HPP
# define GAME_STATE_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <deque>
# include <map>
# include <optional>
# include <string>
# include <vector>

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

// One line of a chat view, ready for display.
struct ChatLine {
	std::string	from;	// sender name, exactly as received
	std::string	text;	// message body, exactly as received
};

/**
 * The local copy of the game, as the client currently believes it to be.
 *
 * Plain data only: no network, no Qt, no logic. The Session is the only one
 * allowed to write into it; the GUI is only allowed to read it.
 *
 * Ids are kept exactly as received (opaque, "npc.guard#3" included): they are
 * what the client must send back. Display is the Catalog's job.
 *
 * The server is the source of truth: this copy is corrected by the
 * resynchronizations the Session asks for (LOOK, INVENTORY, STATUS).
 */
struct GameState {
	enum class Phase { DISCONNECTED, CONNECTED, AUTHENTICATED, TERMINATED };   // RFC §2.2

	// Connection
	Phase		phase = Phase::DISCONNECTED;
	std::string	me;	// our player name, once connected

	// The last ERR received, as "code NAME" (for instance "201 NAME_IN_USE").
	// Cleared when a command is sent: a new attempt must not inherit the
	// failure of the previous one. The connection screen shows it.
	std::optional<std::string>	last_error;

	// The room we are in, as last seen
	struct Room {
		std::string	id;				// "loc.square"
		std::string	name;			// "Square", as sent by the server
		std::string	description;	// as sent by the server
		std::map<std::string, std::string>	exits;	// "north" -> "loc.village"
	} room;

	// Contents of the room, as last announced. Ids only: the Catalog names them.
	// They can be stale (another player picking an item up sends no event):
	// the LOOK that follows an ERR 404, or a periodic LOOK, corrects them.
	std::vector<std::string>	players;	// in the room; players.size() is the room count
	std::vector<std::string>	items;		// on the ground
	std::vector<std::string>	npcs;		// in the room
	std::vector<std::string>	inventory;	// what we carry
	// ^^ These are ids, not names

	// Our own status. Never computed locally: it comes from STATUS or from a
	// combat response, the server alone knows about armour, dodges and crits.
	int				hp = 0;
	int				max_hp = 0;
	std::string		status;				// healthy, wounded, combat, dead
	int				server_players = 0;	// connected on the whole server

	// Combat, as last announced. my_turn is written by comparing the name in
	// EVT COMBAT TURN with me; seconds_left is frozen, not a local countdown.
	struct Combat {
		bool		active = false;
		std::string	enemy;          // id or name, as received
		int			round = 0;
		bool		my_turn = false;
		int			seconds_left = 0;
	} combat;

	// Group. optional rather than an empty string: "in no group" and "in the
	// group named ''" must not be the same state.
	std::optional<std::string>	group;          // none: we are in no group
	std::optional<std::string>	pending_invite; // none: nobody invited us; set: the inviting leader

	// One quest, as the server lists it. Status is "active" or "completed";
	// progress is already written for display ("1/3").
	struct Quest {
		std::string	id;			// "fetch_shield"
		std::string	name;		// as sent by the server
		std::string	status;		// QUEST_STATUS_ACTIVE or QUEST_STATUS_COMPLETED
		std::string	progress;	// "1/3"
	};

	// Our quests, active and completed, as last announced by QUESTS.
	// The server sends no event when a quest changes: the Session asks again
	// after every QUEST, which is when a quest starts or is handed in.
	std::vector<Quest>	quests;

	// Chat, one history per scope. Chat never goes into the journal: the
	// subject requires two separate views, and the journal is technical.
	std::vector<ChatLine>	chat_global;
	std::vector<ChatLine>	chat_room;
	std::vector<ChatLine>	chat_group;

	// What the npcs said to us (TALK, and the QUEST texts), one line each as
	// "<npc name>: <text>". Separate from the journal, which is technical.
	std::deque<std::string>	dialogue;

	// Technical journal, bounded so a long session cannot eat the memory.
	// deque: removing from the front is constant time.
	std::deque<std::string>	log;
};

#endif
