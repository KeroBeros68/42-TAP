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

	// Chat, one history per scope. Chat never goes into the journal: the
	// subject requires two separate views, and the journal is technical.
	std::vector<ChatLine>	chat_global;
	std::vector<ChatLine>	chat_room;
	std::vector<ChatLine>	chat_group;

	// Technical journal, bounded so a long session cannot eat the memory.
	// deque: removing from the front is constant time.
	std::deque<std::string>	log;
};

#endif
