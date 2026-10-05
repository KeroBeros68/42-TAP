#include "parser.hpp"

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
		std::cerr << "[WARN] npc.json: no \"npcs\" object found, no NPC loaded" << std::endl;
	}
}

static void readRoom(RoomMap& rooms, json& data) {
	if (data.contains("rooms") && data["rooms"].is_object()) {
		for (const auto& [room_id, room] : data["rooms"].items()) {

			std::vector<std::string> items;
			if (room.contains("items") && room["items"].is_array()) {
				for (const auto& item : room["items"]) {
				   items.push_back(item.get<std::string>());
				}
			}

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
				items
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

std::string serializeLook(const Room& room, const std::vector<std::string>& players, const std::vector<std::string>& npcs) {
	nlohmann::ordered_json look;
	look["room"]["id"] = room.id();
	look["room"]["name"] = room.name();
	look["room"]["description"] = room.description();
	look["room"]["exits"] = nlohmann::ordered_json::object();
	for (const auto& [direction, target] : room.exits())
		look["room"]["exits"][direction] = target;
	look["players"] = players;
	look["items"] = room.items();
	look["npcs"] = npcs;

	return look.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace);
}
