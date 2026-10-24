#ifndef PARSER_HPP
# define PARSER_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../entities/item.hpp"
# include "../entities/npc.hpp"
# include "room.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Spawns -------------------------------------------------------------------
# define MAX_SPAWN_COUNT 100	// largest number of objects created by one spawn

// -- Data file keys -----------------------------------------------------------
# define JSON_NPCS				"npcs"				// list of the npcs of npc.json
# define JSON_NPC				"npc"				// npc of a spawn
# define JSON_DIALOGUE			"dialogue"			// lines an npc says
# define JSON_ROLE				"role"				// role of an npc
# define JSON_STATS				"stats"				// combat stats of an enemy
# define JSON_HP				"hp"				// hit points (also those of the respawn)
# define JSON_ATTACK			"attack"			// attack of an enemy
# define JSON_DEFENSE			"defense"			// defense of an enemy
# define JSON_HIT				"hit"				// accuracy of an enemy
# define JSON_FLEE				"flee"				// evasion of an enemy
# define JSON_LOOT				"loot"				// ids of the items an enemy drops
# define JSON_NAME				"name"				// name of an npc, an item or a room
# define JSON_DESCRIPTION		"description"		// description of an npc, an item or a room
# define JSON_ITEMS				"items"				// items of items.json, or items on the floor of a room
# define JSON_ITEM				"item"				// item of a spawn
# define JSON_OBTAINABLE		"obtainable"		// the item can be taken
# define JSON_ROOMS				"rooms"				// list of the rooms of room.json
# define JSON_SPAWNS			"spawns"			// npcs created in a room
# define JSON_EXITS				"exits"				// exits of a room
# define JSON_COUNT				"count"				// number of objects created by a spawn
# define JSON_RESPAWN_SECONDS	"respawn_seconds"	// delay before a spawned object comes back
# define JSON_EFFECT			"effect"			// what a room does to the players in it
# define JSON_TYPE				"type"				// kind of an effect
# define JSON_HEALING			"healing"			// type of effect: gives hit points
# define JSON_AMOUNT			"amount"			// hit points given by an effect, each time
# define JSON_INTERVAL_SECONDS	"interval_seconds"	// seconds between two times an effect works
# define JSON_START_ROOM		"start_room"		// room where a new player starts
# define JSON_RESPAWN			"respawn"			// where and with how many hit points a dead player comes back
# define JSON_ROOM				"room"				// room of the respawn

// -- Data file errors ---------------------------------------------------------
# define ERR_NPC_FILE			"invalid npc.json: "	// beginning of the errors about npc.json
# define ERR_ITEM_FILE			"invalid items.json: "	// beginning of the errors about items.json
# define ERR_ROOM_FILE			"invalid room.json: "	// beginning of the errors about room.json

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Reads the definitions of the npcs from their file.
 * Throws std::runtime_error if the file cannot be opened or its content is invalid.
 *
 * @param path path of npc.json
 * @param npcs receives the definitions
 */
void parseNpcs(const std::string& path, NpcDb& npcs);

/**
 * Reads the definitions of the items from their file.
 * Throws std::runtime_error if the file cannot be opened or its content is invalid.
 *
 * @param path  path of items.json
 * @param items receives the definitions
 */
void parseItems(const std::string& path, ItemDb& items);

/**
 * Reads the rooms, the start room and the respawn from their file.
 * Throws std::runtime_error if the file cannot be opened or its content is invalid.
 *
 * @param path         path of room.json
 * @param rooms        receives the rooms
 * @param start_room   receives the id of the room where a new player starts
 * @param respawn_room receives the id of the room where a dead player comes back
 * @param respawn_life receives his hit points when he comes back
 */
void parseRooms(const std::string& path, RoomMap& rooms, std::string& start_room, std::string& respawn_room, int& respawn_life);

#endif
