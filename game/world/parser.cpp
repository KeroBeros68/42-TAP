#include "parser.hpp"
#include "../../shared/logger.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "../../third_party/json.hpp"

using json = nlohmann::json;

static void readNpc(NpcDb& database, json& data) {
	if (data.contains(JSON_NPCS) && data[JSON_NPCS].is_object()) {
		for (const auto& [npc_id, npc] : data[JSON_NPCS].items()) {

			std::vector<std::string> dial;
			if (npc.contains(JSON_DIALOGUE) && npc[JSON_DIALOGUE].is_array()) {
				for (const auto& line : npc[JSON_DIALOGUE]) {
				   dial.push_back(line.get<std::string>());
				}
			}

			const std::string role = npc.at(JSON_ROLE).get<std::string>();

			Stats stats;
			if (role == NPC_ROLE_ENEMY) {
				const json& s = npc.at(JSON_STATS);
				stats.max_hp = s.at(JSON_HP).get<int>();
				stats.hp = stats.max_hp;
				stats.attack = s.at(JSON_ATTACK).get<int>();
				stats.defense = s.at(JSON_DEFENSE).get<int>();
				stats.hit = s.at(JSON_HIT).get<int>();
				stats.flee = s.at(JSON_FLEE).get<int>();
				if (stats.hp <= 0 || stats.attack < 0 || stats.defense < 0 || stats.hit < 0 || stats.flee < 0)
					throw std::runtime_error(ERR_NPC_FILE + npc_id + ": hp must be > 0, attack, defense, hit and flee >= 0");
			}

			std::vector<std::string> loot;
			if (npc.contains(JSON_LOOT) && npc[JSON_LOOT].is_array() && !npc[JSON_LOOT].empty()) {
				for (const auto& item : npc[JSON_LOOT]) {
					loot.push_back(item.get<std::string>());
				}
			}

			NpcDef def(npc_id,
				npc.at(JSON_NAME).get<std::string>(),
				role,
				npc.at(JSON_DESCRIPTION).get<std::string>(),
				dial,
				stats,
				loot
			);

			database.add(def);
		}
	} else {
		LOG_WARN("no \"npcs\" object found in npc.json, no NPC loaded");
	}
}

static void readItems(ItemDb& database, json& data) {
	if (data.contains(JSON_ITEMS) && data[JSON_ITEMS].is_object()) {
		for (const auto& [item_id, item] : data[JSON_ITEMS].items()) {
			ItemDef def(item_id,
				item.at(JSON_NAME).get<std::string>(),
				item.at(JSON_DESCRIPTION).get<std::string>(),
				item.at(JSON_OBTAINABLE).get<bool>()
			);

			database.add(def);
		}
	} else {
		LOG_WARN("no \"items\" object found in items.json, no item loaded");
	}
}

static std::vector<Spawn> readSpawns(const json& entries, const std::string& room_id, const std::string& kind) {
	std::vector<Spawn> spawns;
	for (const auto& entry : entries) {
		if (!entry.is_object())
			throw std::runtime_error(ERR_ROOM_FILE + room_id + ": each " + kind + " entry must be an object");

		Spawn spawn;
		spawn.id = entry.at(kind).get<std::string>();
		spawn.count = entry.value(JSON_COUNT, 1);
		spawn.respawn_seconds = entry.value(JSON_RESPAWN_SECONDS, 0);
		if (spawn.id.empty())
			throw std::runtime_error(ERR_ROOM_FILE + room_id + ": a " + kind + " entry has no id");
		if (spawn.count < 1 || spawn.count > MAX_SPAWN_COUNT)
			throw std::runtime_error(ERR_ROOM_FILE + room_id + ": " + kind + " " + spawn.id + ": count must be between 1 and " + std::to_string(MAX_SPAWN_COUNT));
		if (spawn.respawn_seconds < 0)
			throw std::runtime_error(ERR_ROOM_FILE + room_id + ": " + kind + " " + spawn.id + ": respawn_seconds can not be negative");
		spawns.push_back(spawn);
	}
	return spawns;
}

static void readRoom(RoomMap& rooms, json& data) {
	if (data.contains(JSON_ROOMS) && data[JSON_ROOMS].is_object()) {
		for (const auto& [room_id, room] : data[JSON_ROOMS].items()) {

			std::vector<Spawn> item_spawns;
			if (room.contains(JSON_ITEMS) && room[JSON_ITEMS].is_array())
				item_spawns = readSpawns(room[JSON_ITEMS], room_id, JSON_ITEM);

			std::vector<Spawn> npc_spawns;
			if (room.contains(JSON_SPAWNS) && room[JSON_SPAWNS].is_array())
				npc_spawns = readSpawns(room[JSON_SPAWNS], room_id, JSON_NPC);

			const auto exits = room.at(JSON_EXITS).get<std::unordered_map<std::string, std::string>>();
			for (const auto& [direction, target] : exits) {
				if (oppositeDirection(direction).empty())
					throw std::runtime_error(ERR_ROOM_FILE + room_id + ": unknown exit direction '" + direction + "'");
				if (target.empty())
					throw std::runtime_error(ERR_ROOM_FILE + room_id + ": exit " + direction + " has no destination");
			}

			Room def(room_id,
				room.at(JSON_NAME).get<std::string>(),
				room.at(JSON_DESCRIPTION).get<std::string>(),
				exits,
				item_spawns,
				npc_spawns
			);

			rooms.emplace(room_id, def);
		}
	}
}

template <typename Read>
static void loadJson(const std::string& path, const char* error_prefix, Read read) {
	std::ifstream file(path);
	if (!file)
		throw std::runtime_error("cannot open " + path);

	try {
		json data = json::parse(file);
		read(data);
	} catch (const json::exception& e) {
		throw std::runtime_error(std::string(error_prefix) + e.what());
	}
}

void parseNpcs(const std::string& path, NpcDb& npcs) {
	loadJson(path, ERR_NPC_FILE, [&](json& data) {
		readNpc(npcs, data);
	});
}

void parseItems(const std::string& path, ItemDb& items) {
	loadJson(path, ERR_ITEM_FILE, [&](json& data) {
		readItems(items, data);
	});
}

void parseRooms(const std::string& path, RoomMap& rooms, std::string& start_room, std::string& respawn_room, int& respawn_life) {
	loadJson(path, ERR_ROOM_FILE, [&](json& data) {
		readRoom(rooms, data);
		start_room = data.at(JSON_START_ROOM).get<std::string>();
		respawn_room = data.at(JSON_RESPAWN).at(JSON_ROOM).get<std::string>();
		respawn_life = data.at(JSON_RESPAWN).at(JSON_HP).get<int>();
	});
}
