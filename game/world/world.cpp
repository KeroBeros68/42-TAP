//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <iostream>
# include <stdexcept>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "world.hpp"
# include "../../shared/logging/logger.hpp"
# include "parser.hpp"
# include "spawn.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Project root -------------------------------------------------------------
#ifndef PROJECT_ROOT
# define PROJECT_ROOT "."	// root of the project, given by the Makefile (the current folder otherwise)
#endif

//##############################################################################
//#                                  HELPERS                                   #
//##############################################################################

static const std::string DATA_DIR = std::string(PROJECT_ROOT) + DATA_DIR_RELATIVE;

/**
 * Checks that every exit leads to a room that exists; a one-way exit is only a warning.
 *
 * @param rooms all the rooms
 */
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

/**
 * Checks that every spawn designates a definition that exists.
 *
 * @param rooms       all the rooms
 * @param kind        "NPC" or "item", for the error
 * @param spawns_of   gives the spawns of a room
 * @param definitions the known definitions, by id
 */
template <typename SpawnsOf, typename Definitions>
static void validateSpawns(const RoomMap& rooms, const std::string& kind, SpawnsOf spawnsOf, const Definitions& definitions) {
	for (const auto& [id, room] : rooms) {
		for (const Spawn& spawn : spawnsOf(room)) {
			if (definitions.find(spawn.id) == definitions.end())
				throw std::runtime_error(ERR_ROOM_FILE + id + ": spawn of unknown " + kind + " '" + spawn.id + "'");
		}
	}
}

/**
 * Checks that a quest uses npcs and items that exist: the giver and the one who receives are npcs, the target is
 * an item that can be taken (to bring) or an enemy (to defeat), and the reward items exist.
 *
 * @param quests the quests
 * @param npcs   the npc definitions
 * @param items  the item definitions
 */
static void validateQuests(const QuestDb& quests, const NpcDb& npcs, const ItemDb& items) {
	for (const auto& [id, quest] : quests.all()) {
		for (const std::string& npc : {quest.giver(), quest.turnIn()}) {
			if (npcs.all().find(npc) == npcs.all().end())
				throw std::runtime_error(ERR_QUEST_FILE + id + ": unknown npc '" + npc + "'");
		}
		if (quest.type() == QuestType::DEFEAT) {
			auto enemy = npcs.all().find(quest.target());
			if (enemy == npcs.all().end() || enemy->second.role() != NPC_ROLE_ENEMY)
				throw std::runtime_error(ERR_QUEST_FILE + id + ": target '" + quest.target() + "' is not an enemy");
		} else {
			auto item = items.all().find(quest.target());
			if (item == items.all().end() || !item->second.isObtainable())
				throw std::runtime_error(ERR_QUEST_FILE + id + ": target '" + quest.target() + "' is not an item that can be taken");
		}
		for (const std::string& reward : quest.reward().items) {
			if (items.all().find(reward) == items.all().end())
				throw std::runtime_error(ERR_QUEST_FILE + id + ": unknown reward item '" + reward + "'");
		}
	}
}

/**
 * Checks that an id is the one of a room.
 *
 * @param rooms all the rooms
 * @param id    the id to check
 * @param role  what the room is for, like "start_room", for the error
 */
static void validateRoomId(const RoomMap& rooms, const std::string& id, const std::string& role) {
	if (rooms.find(id) == rooms.end())
		throw std::runtime_error(ERR_ROOM_FILE + role + " '" + id + "' is not a room");
}

/**
 * Creates the objects of every spawn of every room and puts them in their room.
 *
 * @param rooms     all the rooms
 * @param database  npcs or items, to create the objects
 * @param spawns_of gives the spawns of a room
 * @param place     puts an object in a room: place(room, object)
 * @return          the number of objects created
 */
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

//##############################################################################
//#                                   WORLD                                    #
//##############################################################################

//######################## CONSTRUCTOR #########################

World::World() {loadWorld();}

//######################### FUNCTIONS ##########################

void World::loadWorld() {
	parseNpcs(DATA_DIR + NPC_FILE, npc_database);
	parseItems(DATA_DIR + ITEM_FILE, item_database);
	parseQuests(DATA_DIR + QUEST_FILE, quest_database);
	validateQuests(quest_database, npc_database, item_database);
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
	LOG_INFO("world loaded", {"npcs", npc_database.all().size()}, {"items", item_database.all().size()}, {"quests", quest_database.all().size()}, {"spawned_npcs", spawned_npc}, {"spawned_items", spawned_item}, {"rooms", rooms.size()}, {"start_room", start_room}, {"respawn_room", respawn_room});
}
