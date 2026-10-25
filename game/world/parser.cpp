//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <fstream>
# include <iostream>
# include <stdexcept>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "parser.hpp"
# include "../../shared/logging/logger.hpp"
# include "../../third_party/json.hpp"

//##############################################################################
//#                                  HELPERS                                   #
//##############################################################################

using json = nlohmann::json;

/**
 * Reads the "npcs" object of npc.json and adds the definitions to the database.
 *
 * @param database receives the definitions
 * @param data     the content of the file
 */
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

/**
 * Reads and checks the effect of an item.
 * Throws std::runtime_error if it is invalid (not an object, unknown type, amount that is not positive).
 *
 * @param entry   the effect, as written in the file
 * @param item_id id of the item, for the errors
 * @return        the effect
 */
static ItemEffect readItemEffect(const json& entry, const std::string& item_id) {
	if (!entry.is_object())
		throw std::runtime_error(ERR_ITEM_FILE + item_id + ": the effect must be an object");

	ItemEffect effect;
	const std::string type = entry.at(JSON_TYPE).get<std::string>();
	if (type == JSON_HEALING)
		effect.type = ItemEffectType::HEALING;
	else
		throw std::runtime_error(ERR_ITEM_FILE + item_id + ": unknown effect type '" + type + "'");
	effect.amount = entry.at(JSON_AMOUNT).get<int>();
	if (effect.amount <= 0)
		throw std::runtime_error(ERR_ITEM_FILE + item_id + ": effect amount must be positive");
	return effect;
}

/**
 * Reads the "items" object of items.json and adds the definitions to the database.
 *
 * @param database receives the definitions
 * @param data     the content of the file
 */
static void readItems(ItemDb& database, json& data) {
	if (data.contains(JSON_ITEMS) && data[JSON_ITEMS].is_object()) {
		for (const auto& [item_id, item] : data[JSON_ITEMS].items()) {
			std::optional<ItemEffect> effect;
			if (item.contains(JSON_EFFECT))
				effect = readItemEffect(item[JSON_EFFECT], item_id);

			ItemDef def(item_id,
				item.at(JSON_NAME).get<std::string>(),
				item.at(JSON_DESCRIPTION).get<std::string>(),
				item.at(JSON_OBTAINABLE).get<bool>(),
				effect
			);

			database.add(def);
		}
	} else {
		LOG_WARN("no \"items\" object found in items.json, no item loaded");
	}
}

/**
 * Reads the type of a quest.
 * Throws std::runtime_error if it is unknown.
 *
 * @param text     the type, as written in the file
 * @param quest_id id of the quest, for the errors
 * @return         the type
 */
static QuestType readQuestType(const std::string& text, const std::string& quest_id) {
	if (text == QUEST_TYPE_DELIVER)
		return QuestType::DELIVER;
	if (text == QUEST_TYPE_DEFEAT)
		return QuestType::DEFEAT;
	if (text == QUEST_TYPE_FETCH)
		return QuestType::FETCH;
	throw std::runtime_error(ERR_QUEST_FILE + quest_id + ": unknown type '" + text + "'");
}

/**
 * Reads the "quests" object of quests.json and adds the definitions to the database.
 * Throws std::runtime_error if a quest is invalid (unknown type, no giver, turn in or target, count under 1, negative heal).
 *
 * @param database receives the definitions
 * @param data     the content of the file
 */
static void readQuests(QuestDb& database, json& data) {
	if (data.contains(JSON_QUESTS) && data[JSON_QUESTS].is_object()) {
		for (const auto& [quest_id, quest] : data[JSON_QUESTS].items()) {
			QuestReward reward;
			if (quest.contains(JSON_REWARD)) {
				const json& r = quest[JSON_REWARD];
				if (r.contains(JSON_ITEMS))
					reward.items = r[JSON_ITEMS].get<std::vector<std::string>>();
				reward.heal = r.value(JSON_HEAL, 0);
			}

			QuestDef def(quest_id,
				quest.at(JSON_NAME).get<std::string>(),
				readQuestType(quest.at(JSON_TYPE).get<std::string>(), quest_id),
				quest.at(JSON_GIVER).get<std::string>(),
				quest.at(JSON_TURN_IN).get<std::string>(),
				quest.at(JSON_TARGET).get<std::string>(),
				quest.at(JSON_COUNT).get<int>(),
				reward,
				quest.at(JSON_TEXT_START).get<std::string>(),
				quest.at(JSON_TEXT_DONE).get<std::string>()
			);

			if (def.giver().empty() || def.turnIn().empty() || def.target().empty())
				throw std::runtime_error(ERR_QUEST_FILE + quest_id + ": giver, turn_in and target are required");
			if (def.count() < 1)
				throw std::runtime_error(ERR_QUEST_FILE + quest_id + ": count must be at least 1");
			if (def.reward().heal < 0)
				throw std::runtime_error(ERR_QUEST_FILE + quest_id + ": heal can not be negative");
			if (!database.add(def))
				throw std::runtime_error(ERR_QUEST_FILE + quest_id + ": duplicate id");
		}
	} else {
		LOG_WARN("no \"quests\" object found in quests.json, no quest loaded");
	}
}

/**
 * Reads and checks the spawns of a room.
 * Throws std::runtime_error if one is invalid (not an object, no id, bad count or delay).
 *
 * @param entries the spawns, as written in the file
 * @param room_id id of the room, for the errors
 * @param kind    JSON_NPC or JSON_ITEM: the key that holds the id
 * @return        the spawns
 */
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

/**
 * Reads and checks the effect of a room.
 * Throws std::runtime_error if it is invalid (unknown type, amount or interval that is not positive).
 *
 * @param entry   the effect, as written in the file
 * @param room_id id of the room, for the errors
 * @return        the effect
 */
static RoomEffect readEffect(const json& entry, const std::string& room_id) {
	if (!entry.is_object())
		throw std::runtime_error(ERR_ROOM_FILE + room_id + ": the effect must be an object");

	RoomEffect effect;
	const std::string type = entry.at(JSON_TYPE).get<std::string>();
	if (type == JSON_HEALING)
		effect.type = RoomEffectType::HEALING;
	else
		throw std::runtime_error(ERR_ROOM_FILE + room_id + ": unknown effect type '" + type + "'");
	effect.amount = entry.at(JSON_AMOUNT).get<int>();
	effect.interval_seconds = entry.at(JSON_INTERVAL_SECONDS).get<int>();
	if (effect.amount <= 0)
		throw std::runtime_error(ERR_ROOM_FILE + room_id + ": effect amount must be positive");
	if (effect.interval_seconds <= 0)
		throw std::runtime_error(ERR_ROOM_FILE + room_id + ": effect interval_seconds must be positive");
	return effect;
}

/**
 * Reads the "rooms" object of room.json and adds the rooms.
 *
 * @param rooms receives the rooms
 * @param data  the content of the file
 */
static void readRoom(RoomMap& rooms, json& data) {
	if (data.contains(JSON_ROOMS) && data[JSON_ROOMS].is_object()) {
		for (const auto& [room_id, room] : data[JSON_ROOMS].items()) {

			std::vector<Spawn> item_spawns;
			if (room.contains(JSON_ITEMS) && room[JSON_ITEMS].is_array())
				item_spawns = readSpawns(room[JSON_ITEMS], room_id, JSON_ITEM);

			std::vector<Spawn> npc_spawns;
			if (room.contains(JSON_SPAWNS) && room[JSON_SPAWNS].is_array())
				npc_spawns = readSpawns(room[JSON_SPAWNS], room_id, JSON_NPC);

			std::optional<RoomEffect> effect;
			if (room.contains(JSON_EFFECT))
				effect = readEffect(room[JSON_EFFECT], room_id);

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
				npc_spawns,
				effect
			);

			rooms.emplace(room_id, def);
		}
	}
}

/**
 * Opens a data file and gives its JSON to a function.
 * Throws std::runtime_error if the file cannot be opened, or if the JSON is invalid (with the prefix in front of the message).
 *
 * @param path         path of the file
 * @param error_prefix beginning of the error messages, like ERR_NPC_FILE
 * @param read         called as read(data) with the content of the file
 */
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

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

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

void parseQuests(const std::string& path, QuestDb& quests) {
	loadJson(path, ERR_QUEST_FILE, [&](json& data) {
		readQuests(quests, data);
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
