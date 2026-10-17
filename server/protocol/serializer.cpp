#include "serializer.hpp"

#include "../../third_party/json.hpp"

using ordered_json = nlohmann::ordered_json;

static std::string dump(const ordered_json& payload) {
	return payload.dump(-1, ' ', false, ordered_json::error_handler_t::replace);
}

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

std::string serializeStatus(const Stats& stats) {
	ordered_json payload;
	payload[FIELD_HP] = stats.hp;
	payload[FIELD_MAX_HP] = stats.max_hp;
	payload[FIELD_STATUS] = statusName(stats.status());
	return dump(payload);
}

std::string serializeAttack(const CombatResult& combat) {
	ordered_json payload;
	payload[FIELD_ATTACKER_HP] = combat.attacker_hp;
	payload[FIELD_TARGET_HP] = combat.defender_hp;
	payload[FIELD_DAMAGE] = combat.damage_to_defender;
	payload[FIELD_STATUS] = statusName(combat.attacker_status);
	return dump(payload);
}
