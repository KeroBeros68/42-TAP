//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "serializer.hpp"
# include "../../third_party/json.hpp"

//##############################################################################
//#                                  HELPERS                                   #
//##############################################################################

using ordered_json = nlohmann::ordered_json;

/**
 * Writes a JSON object on one line, without spaces; a byte that is not valid UTF-8 is replaced.
 *
 * @param payload the object
 * @return        the JSON text
 */
static std::string dump(const ordered_json& payload) {
	return payload.dump(-1, ' ', false, ordered_json::error_handler_t::replace);
}

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

std::string serializeLook(const LookData& look) {
	const Room& room = *look.room;
	ordered_json payload;
	payload[FIELD_ROOM][FIELD_ID] = room.id();
	payload[FIELD_ROOM][FIELD_NAME] = room.name();
	payload[FIELD_ROOM][FIELD_DESCRIPTION] = room.description();
	payload[FIELD_ROOM][FIELD_EXITS] = ordered_json::object();
	for (const auto& [direction, target] : room.exits())
		payload[FIELD_ROOM][FIELD_EXITS][direction] = target;
	payload[FIELD_PLAYERS] = look.players;
	payload[FIELD_ITEMS] = room.itemDefinitionIds();
	payload[FIELD_NPCS] = room.npcDefinitionIds();
	return dump(payload);
}

std::string serializeStatus(const Stats& stats, const std::optional<CombatView>& combat) {
	ordered_json payload;
	payload[FIELD_HP] = stats.hp;
	payload[FIELD_MAX_HP] = stats.max_hp;
	payload[FIELD_STATUS] = statusName(stats.status());
	if (combat) {
		payload[FIELD_ROUND] = combat->round;
		payload[FIELD_ENEMY] = combat->enemy_id;
		payload[FIELD_TURN] = combat->turn;
		payload[FIELD_YOUR_TURN] = combat->your_turn;
		payload[FIELD_TURN_SECONDS_LEFT] = combat->seconds_left;
	}
	return dump(payload);
}

std::string serializeInventory(const std::vector<std::string>& ids) {
	return dump(ordered_json(ids));
}

/**
 * Gives the word of the protocol for the state of a quest.
 *
 * @param state the state
 * @return      QUEST_STATUS_ACTIVE or QUEST_STATUS_COMPLETED
 */
static const char* questStatusName(QuestState state) {
	return state == QuestState::COMPLETED ? QUEST_STATUS_COMPLETED : QUEST_STATUS_ACTIVE;
}

/**
 * Writes the progress of a quest, like "1/3".
 *
 * @param view the quest
 * @return     what is done over what is asked
 */
static std::string questProgress(const QuestView& view) {
	return std::to_string(view.current) + "/" + std::to_string(view.def->count());
}

/**
 * Writes a reward as one text, like the RFC does: the ids of the items, then the hit points, separated by commas.
 *
 * @param reward the reward
 * @return       like "item.bread,heal=20" (an empty text if there is nothing)
 */
static std::string questRewardText(const QuestReward& reward) {
	std::string text;
	for (const std::string& item : reward.items)
		text += (text.empty() ? "" : ",") + item;
	if (reward.heal > 0)
		text += std::string(text.empty() ? "" : ",") + "heal=" + std::to_string(reward.heal);
	return text;
}

std::string serializeQuest(const QuestReply& reply) {
	const QuestDef& def = *reply.view.def;
	ordered_json payload;
	payload[FIELD_QUEST_ID] = def.id();
	payload[FIELD_NAME] = def.name();
	payload[FIELD_STATUS] = questStatusName(reply.view.state);
	payload[FIELD_DESCRIPTION] = def.textStart();
	payload[FIELD_PROGRESS] = questProgress(reply.view);
	payload[FIELD_REWARD] = questRewardText(def.reward());
	payload[FIELD_REWARD_ITEMS] = def.reward().items;
	payload[FIELD_REWARD_HEAL] = def.reward().heal;
	if (reply.status == QuestStatus::COMPLETED)
		payload[FIELD_TEXT_DONE] = def.textDone();
	return dump(payload);
}

std::string serializeQuests(const std::vector<QuestView>& quests) {
	ordered_json payload = ordered_json::array();
	for (const QuestView& view : quests) {
		payload.push_back({{FIELD_QUEST_ID, view.def->id()}, {FIELD_NAME, view.def->name()},
			{FIELD_STATUS, questStatusName(view.state)}, {FIELD_PROGRESS, questProgress(view)}});
	}
	return dump(payload);
}

std::string serializeAttack(const CombatReport& report) {
	ordered_json turns = ordered_json::array();
	for (const ReportTurn& turn : report.turns) {
		turns.push_back({{FIELD_ACTOR, turn.actor}, {FIELD_TARGET, turn.target}, {FIELD_ROUND, turn.round},
			{FIELD_ACTION, actionName(turn.event.action)}, {FIELD_MISSED, turn.event.missed}, {FIELD_DAMAGE, turn.event.damage}});
	}
	ordered_json payload;
	payload[FIELD_ATTACKER_HP] = report.player_hp;
	payload[FIELD_TARGET_HP] = report.enemy_hp;
	payload[FIELD_DAMAGE] = report.damage;
	payload[FIELD_STATUS] = statusName(report.player_status);
	payload[FIELD_ROUND] = report.round;
	payload[FIELD_ESCAPED] = report.escaped;
	payload[FIELD_QUEUED] = report.status == CombatStatus::QUEUED;
	payload[FIELD_TURNS] = turns;
	return dump(payload);
}
