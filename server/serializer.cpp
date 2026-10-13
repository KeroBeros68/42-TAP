#include "serializer.hpp"

#include "../third_party/json.hpp"

using ordered_json = nlohmann::ordered_json;

static std::string dump(const ordered_json& payload) {
	return payload.dump(-1, ' ', false, ordered_json::error_handler_t::replace);
}

std::string serializeLook(const LookData& look) {
	const Room& room = *look.room;
	ordered_json payload;
	payload["room"]["id"] = room.id();
	payload["room"]["name"] = room.name();
	payload["room"]["description"] = room.description();
	payload["room"]["exits"] = ordered_json::object();
	for (const auto& [direction, target] : room.exits())
		payload["room"]["exits"][direction] = target;
	payload["players"] = look.players;
	payload["items"] = room.items();
	payload["npcs"] = room.npcs();
	return dump(payload);
}

std::string serializeStatus(const Stats& stats) {
	ordered_json payload;
	payload["hp"] = stats.hp;
	payload["max_hp"] = stats.max_hp;
	payload["status"] = statusName(stats.status());
	return dump(payload);
}

std::string serializeAttack(const CombatResult& combat) {
	ordered_json payload;
	payload["attacker_hp"] = combat.attacker_hp;
	payload["target_hp"] = combat.defender_hp;
	payload["damage"] = combat.damage_to_defender;
	payload["status"] = statusName(combat.attacker_status);
	return dump(payload);
}
