#include "world.hpp"

#include <iostream>
#include <stdexcept>

#include "parser.hpp"

// Absolute path of the project root, given by the Makefile (-DPROJECT_ROOT="...").
// Without it, the data files are searched from the current directory.
#ifndef PROJECT_ROOT
# define PROJECT_ROOT "."
#endif

static const std::string DATA_DIR = std::string(PROJECT_ROOT) + "/game/ressources/";

World::World() {loadWorld();}

static void validateExits(const RoomMap& rooms) {
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

static void validateRoomId(const RoomMap& rooms, const std::string& id, const std::string& role) {
	if (rooms.find(id) == rooms.end())
		throw std::runtime_error("invalid room.json: " + role + " '" + id + "' is not a room");
}

void World::loadWorld() {
	parseNpcs(DATA_DIR + "npc.json", npc_database);
	parseRooms(DATA_DIR + "room.json", rooms, start_room, respawn_room);
	validateExits(rooms);
	validateRoomId(rooms, start_room, "start_room");
	validateRoomId(rooms, respawn_room, "respawn room");
}

std::string World::look(const std::string& room_id, const std::vector<std::string>& players) const {
	return serializeLook(rooms.at(room_id), players, {});
}
