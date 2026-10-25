//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <algorithm>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "combat_manager.hpp"
# include "../../shared/logging/logger.hpp"
# include "../utils/lookup.hpp"

//##############################################################################
//#                                  HELPERS                                   #
//##############################################################################

/**
 * Joins words with the separator of the protocol.
 *
 * @param parts the words
 * @return      the line
 */
static std::string words(const std::vector<std::string>& parts) {
	std::string line;
	for (const std::string& part : parts)
		line += (line.empty() ? "" : std::string(1, CMD_SEPARATOR)) + part;
	return line;
}

/**
 * Writes the result of a turn for an event.
 *
 * @param event the turn
 * @return      the damage, "miss", "escaped" or "failed"
 */
static std::string turnResult(const CombatEvent& event) {
	if (event.action == CombatAction::FLEE)
		return event.missed ? EVT_FAILED : EVT_ESCAPED;
	if (event.action == CombatAction::ATTACK && event.missed)
		return EVT_MISS;
	return std::to_string(event.damage);
}

//##############################################################################
//#                               COMBAT MANAGER                               #
//##############################################################################

//######################## CONSTRUCTOR #########################

CombatManager::CombatManager(CombatHost& host) : _host(host) {}

//########################## FIGHTERS ##########################

StatsOf CombatManager::statsOf(Npc* npc) {
	return [this, npc](long long id) -> Stats* {
		if (id == ENEMY_ID)
			return &npc->stats();
		Player* player = _host.findPlayer(id);
		return player ? &player->stats() : nullptr;
	};
}

std::string CombatManager::fighterName(long long id, const std::string& enemy_id) const {
	if (id == ENEMY_ID)
		return enemy_id;
	const Player* player = _host.findPlayer(id);
	return player ? player->name() : std::string();
}

//########################### EVENTS ###########################

void CombatManager::emitRoomCombat(const Room& room, long long except_id, const std::vector<std::string>& parts) {
	std::vector<std::string> all = {EVT_COMBAT};
	all.insert(all.end(), parts.begin(), parts.end());
	_host.events().toOthers(room, except_id, eventLine(EVT_ROOM, words(all)));
}

void CombatManager::emitCombatResolved(const Room& room) {
	_host.events().toRoom(room, eventLine(EVT_ROOM, std::string(EVT_COMBAT) + CMD_SEPARATOR + EVT_COMBAT_RESOLVED));
}

//########################### TURNS ############################

CombatReport CombatManager::continueFight(long long player_id, CombatAction action, const char* name) {
	Presence here = _host.locate(player_id, name);
	if (!here)
		return {CombatStatus::NOT_FOUND};
	const auto fighting = _fighting.find(player_id);
	if (fighting == _fighting.end())
		return {CombatStatus::NOT_IN_COMBAT};
	Fight& fight = _fights.at(fighting->second);
	Npc* npc = here.room->findNpc(fight.encounter.enemy_id);
	if (!npc) {
		dropFight(fight.encounter.enemy_id);
		return {CombatStatus::NOT_FOUND};
	}
	return takeTurn(here, fight, npc, action);
}

CombatReport CombatManager::takeTurn(Presence& here, Fight& fight, Npc* npc, CombatAction action) {
	const long long player_id = here.player->id();
	const StatsOf stats = statsOf(npc);

	Steps steps = advance(fight.encounter, stats);
	CombatReport report;
	if (!steps.over && currentActor(fight.encounter, stats) == player_id) {
		report.round = fight.encounter.round;
		mergeSteps(steps, act(fight.encounter, player_id, action, stats));
	} else if (!steps.over) {
		report.status = CombatStatus::NOT_YOUR_TURN;
	}
	report.player_hp = here.player->stats().hp;
	report.enemy_hp = npc->stats().hp;
	report.player_status = here.player->stats().status();
	settle(fight, npc, *here.room, steps, player_id, &report);
	return report;
}

void CombatManager::settle(Fight& fight, Npc* npc, Room& room, const Steps& steps, long long requester_id, CombatReport* report) {
	const std::string enemy_id = fight.encounter.enemy_id;

	for (const Turn& turn : steps.turns) {
		const std::string actor = fighterName(turn.actor, enemy_id);
		const std::string target = fighterName(turn.target, enemy_id);
		LOG_INFO("combat turn", {"room", room.id()}, {"npc", enemy_id}, {"round", turn.round}, {"actor", actor}, {"target", target},
			{"action", actionName(turn.event.action)}, {"missed", turn.event.missed}, {"damage", turn.event.damage});
		emitRoomCombat(room, requester_id, {EVT_COMBAT_ROUND, actor, enemy_id, std::to_string(turn.round), actionName(turn.event.action), target, turnResult(turn.event)});
		if (report) {
			report->turns.push_back({actor, target, turn.round, turn.event});
			if (turn.actor == requester_id)
				report->damage += turn.event.damage;
		}
	}
	if (report)
		report->escaped = std::find(steps.escaped.begin(), steps.escaped.end(), requester_id) != steps.escaped.end();

	for (long long id : steps.dead)
		playerDied(id, enemy_id, requester_id);
	for (long long id : steps.escaped)
		playerFled(id, enemy_id, requester_id);

	if (steps.enemy_dead) {
		std::string killer;
		for (const Turn& turn : steps.turns) {
			if (turn.actor != ENEMY_ID)
				killer = fighterName(turn.actor, enemy_id);
		}
		const std::vector<long long> participants = fight.encounter.participants;
		for (long long id : participants)
			emitRoomCombat(room, requester_id, {EVT_COMBAT_END, fighterName(id, enemy_id), enemy_id, EVT_VICTORY});
		dropFight(enemy_id);
		_host.enemyDied(room, npc, killer, participants);
		// Last, once the enemy is out of the room and its loot is on the floor:
		// the room changed for everybody, the killer included, so this one goes
		// to the whole room (the END above skip the killer, who has his answer).
		emitCombatResolved(room);
	} else if (steps.over) {
		dropFight(enemy_id);
		emitCombatResolved(room);
	} else {
		announceTurn(fight, room);
	}
}

void CombatManager::announceTurn(Fight& fight, const Room& room) {
	const long long actor = peekActor(fight.encounter);
	if (actor == fight.announced_actor && fight.encounter.round == fight.announced_round)
		return;
	fight.announced_actor = actor;
	fight.announced_round = fight.encounter.round;
	fight.deadline = std::chrono::steady_clock::now() + std::chrono::seconds(COMBAT_TURN_SECONDS);
	emitRoomCombat(room, NO_ACTOR, {EVT_COMBAT_TURN, fighterName(peekActor(fight.encounter), fight.encounter.enemy_id),
		fight.encounter.enemy_id, std::to_string(COMBAT_TURN_SECONDS)});
}

//########################## ENDINGS ###########################

void CombatManager::dropFight(const std::string& enemy_id) {
	const auto fight = _fights.find(enemy_id);
	if (fight == _fights.end())
		return;
	for (long long id : fight->second.encounter.participants) {
		_fighting.erase(id);
		if (Player* player = _host.findPlayer(id))
			player->stats().in_combat = false;
	}
	if (Room* room = _host.findRoom(fight->second.room_id)) {
		if (Npc* npc = room->findNpc(enemy_id))
			npc->stats().in_combat = false;
	}
	_fights.erase(fight);
}

void CombatManager::playerDied(long long player_id, const std::string& enemy_id, long long requester_id) {
	Presence here = _host.locate(player_id, "died");
	if (!here)
		return;
	LOG_INFO("player died", LOG_HERE(here), {"npc", enemy_id});
	_fighting.erase(player_id);
	emitRoomCombat(*here.room, requester_id, {EVT_COMBAT_END, here.player->name(), enemy_id, EVT_DEFEAT});
	_host.teleport(player_id, _host.respawnRoom());
	here.player->stats().revive(_host.respawnLife());
	LOG_INFO("player respawned", {"client", player_id}, {"player", here.player->name()}, {"room", _host.respawnRoom()}, {"hp", here.player->stats().hp});
}

void CombatManager::playerFled(long long player_id, const std::string& enemy_id, long long requester_id) {
	Presence here = _host.locate(player_id, "fled");
	if (!here)
		return;
	LOG_INFO("player fled", LOG_HERE(here));
	_fighting.erase(player_id);
	emitRoomCombat(*here.room, requester_id, {EVT_COMBAT_END, here.player->name(), enemy_id, EVT_ESCAPED});
	const std::vector<std::string> directions = lookup::sortedIds(here.room->exits());
	if (directions.empty())
		return;
	_host.teleport(player_id, here.room->exits().at(directions[randomIndex(directions.size())]));
}

//########################## COMMANDS ##########################

CombatReport CombatManager::attack(long long player_id, const std::string& target) {
	Presence here = _host.locate(player_id, "attack");
	if (!here)
		return {CombatStatus::NOT_FOUND};

	Npc* npc = nullptr;
	Fight* fight = nullptr;
	const auto fighting = _fighting.find(player_id);
	if (fighting != _fighting.end()) {
		fight = &_fights.at(fighting->second);
		npc = here.room->findNpc(fight->encounter.enemy_id);
		if (!npc) {
			dropFight(fight->encounter.enemy_id);
			return {CombatStatus::NOT_FOUND};
		}
		if (!target.empty() && here.room->findNpc(target) != npc)
			return {CombatStatus::ANOTHER_ENEMY};
	} else {
		npc = here.room->findNpc(target);
		if (!npc) {
			LOG_DEBUG("no such NPC in the room", LOG_HERE(here), {"target", target});
			return {CombatStatus::NOT_FOUND};
		}
		if (npc->definition().role() != NPC_ROLE_ENEMY) {
			LOG_DEBUG("player tried to attack a non-hostile NPC", LOG_HERE(here), {"npc", npc->id()});
			return {CombatStatus::NOT_HOSTILE};
		}
		auto found = _fights.find(npc->id());
		const bool created = (found == _fights.end());
		if (created) {
			found = _fights.emplace(npc->id(), Fight{startEncounter(npc->id()), here.room->id(), std::chrono::steady_clock::time_point::max()}).first;
			LOG_INFO("combat started", LOG_HERE(here), {"npc", npc->id()});
			emitRoomCombat(*here.room, player_id, {EVT_COMBAT_START, here.player->name(), npc->id()});
		} else {
			LOG_INFO("player joined a combat", LOG_HERE(here), {"npc", npc->id()});
			emitRoomCombat(*here.room, player_id, {EVT_COMBAT_JOIN, here.player->name(), npc->id()});
		}
		fight = &found->second;
		join(fight->encounter, player_id, statsOf(npc));
		_fighting[player_id] = npc->id();
	}
	return takeTurn(here, *fight, npc, CombatAction::ATTACK);
}

CombatReport CombatManager::defend(long long player_id) {
	return continueFight(player_id, CombatAction::DEFEND, "defend");
}

CombatReport CombatManager::flee(long long player_id) {
	return continueFight(player_id, CombatAction::FLEE, "flee");
}

//########################## QUERIES ###########################

bool CombatManager::inCombat(long long player_id) const {
	return _fighting.find(player_id) != _fighting.end();
}

std::optional<CombatView> CombatManager::view(long long player_id) const {
	const auto fighting = _fighting.find(player_id);
	if (fighting == _fighting.end())
		return std::nullopt;
	const Fight& fight = _fights.at(fighting->second);
	const long long actor = peekActor(fight.encounter);
	const auto left = std::chrono::duration_cast<std::chrono::seconds>(fight.deadline - std::chrono::steady_clock::now()).count() + 1;
	return CombatView{fight.encounter.enemy_id, fight.encounter.round, fighterName(actor, fight.encounter.enemy_id),
		actor == player_id, static_cast<int>(std::max<long long>(0, std::min<long long>(left, COMBAT_TURN_SECONDS)))};
}

//########################## UPDATES ###########################

void CombatManager::abandon(long long player_id) {
	const auto fighting = _fighting.find(player_id);
	if (fighting == _fighting.end())
		return;
	const std::string enemy_id = fighting->second;
	_fighting.erase(fighting);
	const auto found = _fights.find(enemy_id);
	if (found == _fights.end())
		return;
	Fight& fight = found->second;
	Room* room = _host.findRoom(fight.room_id);
	Npc* npc = room ? room->findNpc(enemy_id) : nullptr;
	if (!npc) {
		dropFight(enemy_id);
		return;
	}
	leave(fight.encounter, player_id, statsOf(npc));
	if (fight.encounter.participants.empty()) {
		dropFight(enemy_id);
		return;
	}
	settle(fight, npc, *room, advance(fight.encounter, statsOf(npc)), NO_ACTOR, nullptr);
}

void CombatManager::update(std::chrono::steady_clock::time_point now) {
	std::vector<std::string> late;
	for (const auto& [enemy_id, fight] : _fights) {
		if (fight.deadline <= now)
			late.push_back(enemy_id);
	}
	for (const std::string& enemy_id : late) {
		const auto found = _fights.find(enemy_id);
		if (found == _fights.end())
			continue;
		Fight& fight = found->second;
		Room* found_room = _host.findRoom(fight.room_id);
		Npc* npc = found_room ? found_room->findNpc(enemy_id) : nullptr;
		if (!npc) {
			dropFight(enemy_id);
			continue;
		}
		Room& room = *found_room;
		const long long player_id = peekActor(fight.encounter);
		if (player_id == NO_ACTOR || player_id == ENEMY_ID) {
			fight.deadline = std::chrono::steady_clock::time_point::max();
			continue;
		}
		LOG_INFO("combat turn timeout", {"room", room.id()}, {"npc", enemy_id}, {"player", fighterName(player_id, enemy_id)});
		emitRoomCombat(room, NO_ACTOR, {EVT_COMBAT_TIMEOUT, fighterName(player_id, enemy_id), enemy_id});
		settle(fight, npc, room, act(fight.encounter, player_id, CombatAction::DEFEND, statsOf(npc)), NO_ACTOR, nullptr);
	}
}
