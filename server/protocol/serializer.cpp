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
	payload[FIELD_ITEMS] = room.items();
	payload[FIELD_NPCS] = room.npcs();
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
	payload[FIELD_TURNS] = turns;
	return dump(payload);
}
