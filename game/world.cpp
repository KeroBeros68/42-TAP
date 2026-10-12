#include "world.hpp"

#include <iostream>
#include <stdexcept>

#include "parser.hpp"
#include "../shared/logger.hpp"

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
				LOG_WARN("one-way exit", {"room", id}, {"direction", direction}, {"to", target});
		}
	}
}

static void validateSpawns(const RoomMap& rooms, const NpcDb& npc_database) {
	for (const auto& [id, room] : rooms) {
		for (const Spawn& spawn : room.spawns()) {
			if (npc_database.npcs().find(spawn.npc) == npc_database.npcs().end())
				throw std::runtime_error("invalid room.json: " + id + ": spawn of unknown NPC '" + spawn.npc + "'");
		}
	}
}

static void validateRoomId(const RoomMap& rooms, const std::string& id, const std::string& role) {
	if (rooms.find(id) == rooms.end())
		throw std::runtime_error("invalid room.json: " + role + " '" + id + "' is not a room");
}

static size_t createNpcs(RoomMap& rooms, const NpcDb& npc_database) {
	size_t created = 0;
	for (auto& [id, room] : rooms) {
		for (const Spawn& spawn : room.spawns()) {
			for (int i = 0; i < spawn.count; i++) {
				room.addNpc(npc_database.instantiate(spawn.npc));
				created++;
			}
		}
	}
	return created;
}

void World::loadWorld() {
	parseNpcs(DATA_DIR + "npc.json", npc_database);
	parseRooms(DATA_DIR + "room.json", rooms, start_room, respawn_room);
	validateExits(rooms);
	validateSpawns(rooms, npc_database);
	size_t spawned = createNpcs(rooms, npc_database);
	validateRoomId(rooms, start_room, "start_room");
	validateRoomId(rooms, respawn_room, "respawn room");
	LOG_INFO("world loaded", {"npcs", npc_database.npcs().size()}, {"spawned_npcs", spawned}, {"rooms", rooms.size()}, {"start_room", start_room}, {"respawn_room", respawn_room});
}

std::string World::look(const std::string& room_id, const std::vector<std::string>& players) const {
	return serializeLook(rooms.at(room_id), players, rooms.at(room_id).npcs());
}
