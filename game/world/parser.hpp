#ifndef PARSER_HPP
# define PARSER_HPP

# include <string>
# include <vector>

# include "../entities/npc.hpp"
# include "../entities/item.hpp"
# include "room.hpp"

# define MAX_SPAWN_COUNT 100

// Keys of the data files (npc.json, items.json, room.json)
# define JSON_NPCS				"npcs"
# define JSON_NPC				"npc"
# define JSON_DIALOGUE			"dialogue"
# define JSON_ROLE				"role"
# define JSON_STATS				"stats"
# define JSON_HP				"hp"
# define JSON_ATTACK			"attack"
# define JSON_DEFENSE			"defense"
# define JSON_HIT				"hit"
# define JSON_FLEE				"flee"
# define JSON_LOOT				"loot"
# define JSON_NAME				"name"
# define JSON_DESCRIPTION		"description"
# define JSON_ITEMS				"items"
# define JSON_ITEM				"item"
# define JSON_OBTAINABLE		"obtainable"
# define JSON_ROOMS				"rooms"
# define JSON_SPAWNS			"spawns"
# define JSON_EXITS				"exits"
# define JSON_COUNT				"count"
# define JSON_RESPAWN_SECONDS	"respawn_seconds"
# define JSON_START_ROOM		"start_room"
# define JSON_RESPAWN			"respawn"
# define JSON_ROOM				"room"

// Beginning of the error messages about a data file
# define ERR_NPC_FILE			"invalid npc.json: "
# define ERR_ITEM_FILE			"invalid items.json: "
# define ERR_ROOM_FILE			"invalid room.json: "

void parseNpcs(const std::string& path, NpcDb& npcs);
void parseItems(const std::string& path, ItemDb& items);
void parseRooms(const std::string& path, RoomMap& rooms, std::string& start_room, std::string& respawn_room, int& respawn_life);

#endif
