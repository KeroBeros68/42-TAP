#ifndef EVENT_HPP
# define EVENT_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../defines.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Room and server events ---------------------------------------------------
# define EVT_ROOM			"ROOM"				// category of the events for the players of a room
# define EVT_STATS			"STATS"				// category of the events about the server
# define EVT_PRESENCE_ENTER	"PRESENCE ENTER"	// a player entered the room
# define EVT_PRESENCE_LEAVE	"PRESENCE LEAVE"	// a player left the room
# define EVT_PLAYERS_FIELD	"players="			// beginning of the number of players

// -- Combat events ------------------------------------------------------------
# define EVT_COMBAT			"COMBAT"	// type of the events about a fight
# define EVT_COMBAT_START	"START"		// a fight begins
# define EVT_COMBAT_ROUND	"ROUND"		// a turn was played
# define EVT_COMBAT_END		"END"		// a player leaves the fight
# define EVT_COMBAT_TURN	"TURN"		// a player has to play
# define EVT_COMBAT_JOIN	"JOIN"		// a player joins a fight
# define EVT_COMBAT_TIMEOUT	"TIMEOUT"	// a turn is played for a player who was too slow
# define EVT_VICTORY		"victory"	// result: the enemy was killed
# define EVT_DEFEAT			"defeat"	// result: the player died
# define EVT_ESCAPED		"escaped"	// result: the player ran away
# define EVT_FAILED			"failed"	// result: the escape failed
# define EVT_MISS			"miss"		// result: the strike missed

// -- Spawn events -------------------------------------------------------------
# define EVT_SPAWN			"SPAWN"	// type of the events about something that comes back
# define EVT_SPAWN_NPC		"npc"	// an npc came back
# define EVT_SPAWN_ITEM		"item"	// an item came back

// -- Chat events --------------------------------------------------------------
# define EVT_GLOBAL			"GLOBAL"	// category of the events for every player
# define EVT_CHAT			"CHAT"		// type of the events about a chat message

// -- Effect events ------------------------------------------------------------
# define EVT_EFFECT			"EFFECT"	// type of the events about the effect of a room on a player
# define EVT_EFFECT_HEALING	"healing"	// the room gave hit points back

// -- Inventory events ---------------------------------------------------------
# define EVT_INVENTORY			"INVENTORY"	// category of the events for one player whose inventory changed without TAKE or DROP
# define EVT_INVENTORY_CHANGED	"CHANGED"	// type: the inventory changed, the client should ask INVENTORY again
# define EVT_INVENTORY_QUEST	"quest"		// reason: items were taken or given by a quest that was handed in
# define EVT_INVENTORY_USE		"use"		// reason: an item was used up

// -- Group events -------------------------------------------------------------
# define EVT_GROUP			"GROUP"		// category of the events for the players of a group
# define EVT_GROUP_INVITE	"INVITE"	// a player invites you
# define EVT_GROUP_JOIN		"JOIN"		// a player joined the group
# define EVT_GROUP_LEAVE	"LEAVE"		// a player left the group

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct Event {
	std::vector<long long> players_ids;
	std::string line;
};

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/** Builds the line of an event, "EVT <category> <data>" (without the line end). */
inline std::string eventLine(const std::string& category, const std::string& data) {
	return std::string(RESPONSE_EVT) + CMD_SEPARATOR + category + CMD_SEPARATOR + data;
}

#endif
