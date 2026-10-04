#include "world.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../shared/defines.hpp"
#include "../third_party/json.hpp"

using json = nlohmann::json;

// Absolute path of the project root, given by the Makefile (-DPROJECT_ROOT="...").
// Without it, the data files are searched from the current directory.
#ifndef PROJECT_ROOT
# define PROJECT_ROOT "."
#endif

static const std::string DATA_DIR = std::string(PROJECT_ROOT) + "/game/ressources/";

World::World() {loadWorld();}

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

static std::string oppositeDirection(const std::string& direction) {
	if (direction == DIR_NORTH)
		return DIR_SOUTH;
	if (direction == DIR_SOUTH)
		return DIR_NORTH;
	if (direction == DIR_EAST)
		return DIR_WEST;
	if (direction == DIR_WEST)
		return DIR_EAST;
	return "";
}

static void readRoom(RoomDb& database, json& data) {
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

			RoomDef def(room_id,
				room.at("name").get<std::string>(),
				room.at("description").get<std::string>(),
				exits,
				items
			);

			database.addRoom(def);
		}
	}
}

static void validateExits(const RoomDb& database) {
	const auto& rooms = database.rooms();
	for (const auto& [id, room] : rooms) {
		for (const auto& [direction, target] : room.exits()) {
			auto destination = rooms.find(target);
			if (destination == rooms.end())
				throw std::runtime_error("invalid room.json: " + id + ": exit " + direction + " leads to unknown room '" + target + "'");

			const auto& back = destination->second.exits();
			auto way_back = back.find(oppositeDirection(direction));
			if (way_back == back.end() || way_back->second != id)
				std::cerr << "[WARN] room.json: " << id << " --" << direction << "--> " << target << " is a one-way exit" << std::endl;
		}
	}
}

void World::loadWorld() {
	const std::string npc_path = DATA_DIR + "npc.json";
	std::ifstream npc_file(npc_path);
	if (!npc_file)
		throw std::runtime_error("cannot open " + npc_path);

	try {
		json npc_data = json::parse(npc_file);
		readNpc(npc_database, npc_data);
	} catch (const json::exception& e) {
		throw std::runtime_error(std::string("invalid npc.json: ") + e.what());
	}

	const std::string room_path = DATA_DIR + "room.json";
	std::ifstream room_file(room_path);
	if (!room_file)
		throw std::runtime_error("cannot open " + room_path);

	try {
		json room_data = json::parse(room_file);
		readRoom(room_database, room_data);
	} catch (const json::exception& e) {
		throw std::runtime_error(std::string("invalid room.json: ") + e.what());
	}
	validateExits(room_database);
}
