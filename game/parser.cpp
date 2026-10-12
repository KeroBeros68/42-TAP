#include "parser.hpp"
#include "../shared/logger.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "../third_party/json.hpp"

using json = nlohmann::json;

static void readNpc(NpcDb& database, json& data) {
	if (data.contains("npcs") && data["npcs"].is_object()) {
		for (const auto& [npc_id, npc] : data["npcs"].items()) {

			std::vector<std::string> dial;
			if (npc.contains("dialogue") && npc["dialogue"].is_array()) {
				for (const auto& line : npc["dialogue"]) {
				   dial.push_back(line.get<std::string>());
				}
			}

			const std::string role = npc.at("role").get<std::string>();

			Stats stats;
			if (role == NPC_ROLE_ENEMY) {
				const json& s = npc.at("stats");
				stats.hp = s.at("hp").get<int>();
				stats.attack = s.at("attack").get<int>();
				stats.defense = s.at("defense").get<int>();
				if (stats.hp <= 0 || stats.attack < 0 || stats.defense < 0)
					throw std::runtime_error("invalid npc.json: " + npc_id + ": hp must be > 0, attack and defense >= 0");
			}

			std::vector<std::string> loot;
			if (npc.contains("loot") && npc["loot"].is_array() && !npc["loot"].empty()) {
				for (const auto& item : npc["loot"]) {
					loot.push_back(item.get<std::string>());
				}
			}

			NpcDef def(npc_id,
				npc.at("name").get<std::string>(),
				role,
				npc.at("description").get<std::string>(),
				dial,
				stats,
				loot
			);

			database.addNpc(def);
		}
	} else {
		LOG_WARN("no \"npcs\" object found in npc.json, no NPC loaded");
	}
}

static void readItems(ItemDb& database, json& data) {
	if (data.contains("items") && data["items"].is_object()) {
		for (const auto& [item_id, item] : data["items"].items()) {
			ItemDef def(item_id,
				item.at("name").get<std::string>(),
				item.at("description").get<std::string>(),
				item.at("obtainable").get<bool>()
			);

			database.addItem(def);
		}
	} else {
		LOG_WARN("no \"items\" object found in items.json, no item loaded");
	}
}

static std::vector<Spawn> readSpawns(const json& entries, const std::string& room_id, const std::string& kind) {
	std::vector<Spawn> spawns;
	for (const auto& entry : entries) {
		if (!entry.is_object())
			throw std::runtime_error("invalid room.json: " + room_id + ": each " + kind + " entry must be an object");

		Spawn spawn;
		spawn.id = entry.at(kind).get<std::string>();
		spawn.count = entry.value("count", 1);
		spawn.respawn_seconds = entry.value("respawn_seconds", 0);
		if (spawn.id.empty())
			throw std::runtime_error("invalid room.json: " + room_id + ": a " + kind + " entry has no id");
		if (spawn.count < 1 || spawn.count > MAX_SPAWN_COUNT)
			throw std::runtime_error("invalid room.json: " + room_id + ": " + kind + " " + spawn.id + ": count must be between 1 and " + std::to_string(MAX_SPAWN_COUNT));
		if (spawn.respawn_seconds < 0)
			throw std::runtime_error("invalid room.json: " + room_id + ": " + kind + " " + spawn.id + ": respawn_seconds can not be negative");
		spawns.push_back(spawn);
	}
	return spawns;
}

static void readRoom(RoomMap& rooms, json& data) {
	if (data.contains("rooms") && data["rooms"].is_object()) {
		for (const auto& [room_id, room] : data["rooms"].items()) {

			std::vector<Spawn> item_spawns;
			if (room.contains("items") && room["items"].is_array())
				item_spawns = readSpawns(room["items"], room_id, "item");

			std::vector<Spawn> npc_spawns;
			if (room.contains("spawns") && room["spawns"].is_array())
				npc_spawns = readSpawns(room["spawns"], room_id, "npc");

			const auto exits = room.at("exits").get<std::unordered_map<std::string, std::string>>();
			for (const auto& [direction, target] : exits) {
				if (oppositeDirection(direction).empty())
					throw std::runtime_error("invalid room.json: " + room_id + ": unknown exit direction '" + direction + "'");
				if (target.empty())
					throw std::runtime_error("invalid room.json: " + room_id + ": exit " + direction + " has no destination");
			}

			Room def(room_id,
				room.at("name").get<std::string>(),
				room.at("description").get<std::string>(),
				exits,
				item_spawns,
				npc_spawns
			);

			rooms.emplace(room_id, def);
		}
	}
}

void parseNpcs(const std::string& path, NpcDb& npcs) {
	std::ifstream file(path);
	if (!file)
		throw std::runtime_error("cannot open " + path);

	try {
		json data = json::parse(file);
		readNpc(npcs, data);
	} catch (const json::exception& e) {
		throw std::runtime_error(std::string("invalid npc.json: ") + e.what());
	}
}

void parseItems(const std::string& path, ItemDb& items) {
	std::ifstream file(path);
	if (!file)
		throw std::runtime_error("cannot open " + path);

	try {
		json data = json::parse(file);
		readItems(items, data);
	} catch (const json::exception& e) {
		throw std::runtime_error(std::string("invalid items.json: ") + e.what());
	}
}

void parseRooms(const std::string& path, RoomMap& rooms, std::string& start_room, std::string& respawn_room) {
	std::ifstream file(path);
	if (!file)
		throw std::runtime_error("cannot open " + path);

	try {
		json data = json::parse(file);
		readRoom(rooms, data);
		start_room = data.at("start_room").get<std::string>();
		respawn_room = data.at("respawn").at("room").get<std::string>();
	} catch (const json::exception& e) {
		throw std::runtime_error(std::string("invalid room.json: ") + e.what());
	}
}

std::string serializeLook(const Room& room, const std::vector<std::string>& players, const std::vector<std::string>& npcs, const std::vector<std::string>& items) {
	nlohmann::ordered_json look;
	look["room"]["id"] = room.id();
	look["room"]["name"] = room.name();
	look["room"]["description"] = room.description();
	look["room"]["exits"] = nlohmann::ordered_json::object();
	for (const auto& [direction, target] : room.exits())
		look["room"]["exits"][direction] = target;
	look["players"] = players;
	look["items"] = items;
	look["npcs"] = npcs;

	return look.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace);
}
