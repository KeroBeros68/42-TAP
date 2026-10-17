#include "world.hpp"

#include <iostream>
#include <stdexcept>

#include "parser.hpp"
#include "spawn.hpp"
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

static const auto npcSpawnsOf = [](const Room& room) -> const std::vector<Spawn>& { return room.npcSpawns(); };
static const auto itemSpawnsOf = [](const Room& room) -> const std::vector<Spawn>& { return room.itemSpawns(); };

template <typename SpawnsOf, typename Definitions>
static void validateSpawns(const RoomMap& rooms, const std::string& kind, SpawnsOf spawnsOf, const Definitions& definitions) {
	for (const auto& [id, room] : rooms) {
		for (const Spawn& spawn : spawnsOf(room)) {
			if (definitions.find(spawn.id) == definitions.end())
				throw std::runtime_error(ERR_ROOM_FILE + id + ": spawn of unknown " + kind + " '" + spawn.id + "'");
		}
	}
}

static void validateRoomId(const RoomMap& rooms, const std::string& id, const std::string& role) {
	if (rooms.find(id) == rooms.end())
		throw std::runtime_error(ERR_ROOM_FILE + role + " '" + id + "' is not a room");
}

template <typename Db, typename SpawnsOf, typename Place>
static size_t createInstances(RoomMap& rooms, const Db& database, SpawnsOf spawnsOf, Place place) {
	size_t created = 0;
	for (auto& [id, room] : rooms) {
		for (const Spawn& spawn : spawnsOf(room)) {
			for (int i = 0; i < spawn.count; i++) {
				spawnInstance(database, spawn.id, id, spawn.respawn_seconds, room, place);
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
	validateSpawns(rooms, "NPC", npcSpawnsOf, npc_database.all());
	validateSpawns(rooms, "item", itemSpawnsOf, item_database.all());
	size_t spawned_npc = createInstances(rooms, npc_database, npcSpawnsOf,
		[](Room& room, const Npc& npc) { room.addNpc(npc); });
	size_t spawned_item = createInstances(rooms, item_database, itemSpawnsOf,
		[](Room& room, const Item& item) { room.inventory().add(item); });
	validateRoomId(rooms, start_room, "start_room");
	validateRoomId(rooms, respawn_room, "respawn room");
	LOG_INFO("world loaded", {"npcs", npc_database.all().size()}, {"items", item_database.all().size()}, {"spawned_npcs", spawned_npc}, {"spawned_items", spawned_item}, {"rooms", rooms.size()}, {"start_room", start_room}, {"respawn_room", respawn_room});
}
