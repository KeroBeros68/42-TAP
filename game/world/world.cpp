#include "world.hpp"

#include <iostream>
#include <stdexcept>

#include "parser.hpp"
#include "../../shared/logger.hpp"

#ifndef PROJECT_ROOT
# define PROJECT_ROOT "."
#endif

static const std::string DATA_DIR = std::string(PROJECT_ROOT) + DATA_DIR_RELATIVE;

World::World() {loadWorld();}

static void validateExits(const RoomMap& rooms) {
	for (const auto& [id, room] : rooms) {
		for (const auto& [direction, target] : room.exits()) {
			auto destination = rooms.find(target);
			if (destination == rooms.end())
				throw std::runtime_error(ERR_ROOM_FILE + id + ": exit " + direction + " leads to unknown room '" + target + "'");

			const auto& back = destination->second.exits();
			auto way_back = back.find(oppositeDirection(direction));
			if (way_back == back.end() || way_back->second != id)
				LOG_WARN("one-way exit", {"room", id}, {"direction", direction}, {"to", target});
		}
	}
}

static void validateNpcSpawns(const RoomMap& rooms, const NpcDb& npc_database) {
	for (const auto& [id, room] : rooms) {
		for (const Spawn& spawn : room.npcSpawns()) {
			if (npc_database.npcs().find(spawn.id) == npc_database.npcs().end())
				throw std::runtime_error(ERR_ROOM_FILE + id + ": spawn of unknown NPC '" + spawn.id + "'");
		}
	}
}

static void validateItemSpawns(const RoomMap& rooms, const ItemDb& item_database) {
	for (const auto& [id, room] : rooms) {
		for (const Spawn& spawn : room.itemSpawns()) {
			if (item_database.items().find(spawn.id) == item_database.items().end())
				throw std::runtime_error(ERR_ROOM_FILE + id + ": unknown item '" + spawn.id + "'");
		}
	}
}

static void validateRoomId(const RoomMap& rooms, const std::string& id, const std::string& role) {
	if (rooms.find(id) == rooms.end())
		throw std::runtime_error(ERR_ROOM_FILE + role + " '" + id + "' is not a room");
}

static size_t createNpcs(RoomMap& rooms, const NpcDb& npc_database) {
	size_t created = 0;
	for (auto& [id, room] : rooms) {
		for (const Spawn& spawn : room.npcSpawns()) {
			for (int i = 0; i < spawn.count; i++) {
				Npc npc = npc_database.instantiate(spawn.id);
				npc.setOrigin(id, spawn.respawn_seconds);
				room.addNpc(npc);
				created++;
			}
		}
	}
	return created;
}

static size_t createItems(RoomMap& rooms, const ItemDb& item_database) {
	size_t created = 0;
	for (auto& [id, room] : rooms) {
		for (const Spawn& spawn : room.itemSpawns()) {
			for (int i = 0; i < spawn.count; i++) {
				Item item = item_database.instantiate(spawn.id);
				item.setOrigin(id, spawn.respawn_seconds);
				room.inventory().add(item);
				created++;
			}
		}
	}
	return created;
}

void World::loadWorld() {
	parseNpcs(DATA_DIR + NPC_FILE, npc_database);
	parseItems(DATA_DIR + ITEM_FILE, item_database);
	parseRooms(DATA_DIR + ROOM_FILE, rooms, start_room, respawn_room, respawn_life);
	validateExits(rooms);
	validateNpcSpawns(rooms, npc_database);
	validateItemSpawns(rooms, item_database);
	size_t spawned_npc = createNpcs(rooms, npc_database);
	size_t spawned_item = createItems(rooms, item_database);
	validateRoomId(rooms, start_room, "start_room");
	validateRoomId(rooms, respawn_room, "respawn room");
	LOG_INFO("world loaded", {"npcs", npc_database.npcs().size()}, {"items", item_database.items().size()}, {"spawned_npcs", spawned_npc}, {"spawned_items", spawned_item}, {"rooms", rooms.size()}, {"start_room", start_room}, {"respawn_room", respawn_room});
}
