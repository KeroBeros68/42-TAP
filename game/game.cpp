#include "game.hpp"
#include "../shared/logger.hpp"
#include "world/spawn.hpp"
#include "utils/lookup.hpp"

#include <iostream>
#include <utility>

Game::Game() {}

void Game::addPlayer(long long id, const std::string& name) {
	Player player(id, name, _world.start_room);
	_players.insert_or_assign(id, player);

	auto room = _world.rooms.find(player.currentMap());
	if (room == _world.rooms.end()) {
		LOG_WARN("unknown map for player", {"client", id}, {"room", player.currentMap()});
		return;
	}
	room->second.addPlayer(id);
	LOG_INFO("player entered room", {"client", id}, {"player", player.name()}, {"room", player.currentMap()});
}

void Game::removePlayer(long long id) {
	auto player = _players.find(id);
	if (player == _players.end())
		return;

	auto room = _world.rooms.find(player->second.currentMap());
	if (room != _world.rooms.end()) {
		room->second.removePlayer(id);
		auto fight = _combats.find(id);
		if (fight != _combats.end()) {
			if (Npc* enemy = room->second.findNpc(fight->second.enemy_id))
				enemy->stats().in_combat = false;
		}
	}
	_combats.erase(id);
	LOG_INFO("player left room", {"client", id}, {"player", player->second.name()}, {"room", player->second.currentMap()});
	_players.erase(player);
}

bool Game::hasPlayer(long long id) const {
	return _players.find(id) != _players.end();
}

bool Game::isNameTaken(const std::string& name) const {
	for (const auto& p : _players) {
		if (p.second.name() == name)
			return true;
	}
	return false;
}

size_t Game::playerCount() const {
	return _players.size();
}

std::vector<long long> Game::playerIds() const {
	std::vector<long long> ids;
	ids.reserve(_players.size());
	for (const auto& p : _players)
		ids.push_back(p.first);
	return ids;
}

const World& Game::world() const {
	return _world;
}

const std::string& Game::playerMap(long long id) const {
	return _players.at(id).currentMap();
}

std::optional<LookData> Game::look(long long player_id) const {
	ConstPresence here = locate(player_id, "look");
	if (!here)
		return std::nullopt;

	LookData data{here.room, {}};
	for (long long id : here.room->currentPlayer()) {
		auto other = _players.find(id);
		if (other != _players.end())
			data.players.push_back(other->second.name());
	}
	return data;
}

Game::ConstPresence Game::locate(long long player_id, const char* action) const {
	auto player = _players.find(player_id);
	if (player == _players.end()) {
		LOG_WARN("unknown player", {"client", player_id}, {"action", action});
		return {};
	}
	auto room = _world.rooms.find(player->second.currentMap());
	if (room == _world.rooms.end()) {
		LOG_ERROR("player is on an unknown room", {"client", player_id}, {"room", player->second.currentMap()}, {"action", action});
		return {};
	}
	return {&player->second, &room->second};
}

Game::Presence Game::locate(long long player_id, const char* action) {
	const ConstPresence here = std::as_const(*this).locate(player_id, action);
	return {const_cast<Player*>(here.player), const_cast<Room*>(here.room)};
}

bool Game::move(long long player_id, const std::string& direction) {
	Presence here = locate(player_id, "move");
	if (!here)
		return false;

	auto exit = here.room->exits().find(direction);
	if (exit == here.room->exits().end()) {
		LOG_DEBUG("no exit in this direction", LOG_HERE(here), {"direction", direction});
		return false;
	}

	const std::string from = here.room->id();
	if (!teleport(player_id, exit->second))
		return false;
	LOG_INFO("player moved", {"client", player_id}, {"player", here.player->name()}, {"from", from}, {"to", exit->second}, {"direction", direction});
	return true;
}

bool Game::teleport(long long player_id, const std::string& target) {
	Presence here = locate(player_id, "teleport");
	if (!here)
		return false;

	auto destination = _world.rooms.find(target);
	if (destination == _world.rooms.end()) {
		LOG_ERROR("teleport to an unknown room", {"client", player_id}, {"room", target});
		return false;
	}

	here.player->stats().in_combat = false;
	_combats.erase(player_id);
	here.room->removePlayer(player_id);
	destination->second.addPlayer(player_id);
	here.player->setCurrentMap(target);
	return true;
}

std::optional<std::string> Game::talk(long long player_id, const std::string& target) const {
	ConstPresence here = locate(player_id, "talk");
	if (!here)
		return std::nullopt;

	const Npc* npc = here.room->findNpc(target);
	if (!npc) {
		LOG_DEBUG("no such NPC in the room", LOG_HERE(here), {"target", target});
		return std::nullopt;
	}

	LOG_INFO("player talked to an npc", LOG_HERE(here), {"npc", npc->id()});
	return npc->talk();
}

TransferResult Game::take(long long player_id, const std::string& target) {
	Presence here = locate(player_id, "take");
	if (!here)
		return {TransferStatus::NotFound, ""};

	const Item* found = here.room->inventory().findItem(target);
	if (!found || !found->definition().isObtainable()) {
		LOG_DEBUG("no such item to take", LOG_HERE(here), {"target", target});
		return {TransferStatus::NotFound, ""};
	}

	const Respawn respawn = Respawn::of(RespawnKind::Item, *found);

	TransferResult result = transfer(here.room->inventory(), here.player->inventory(), found->id());
	if (result.status == TransferStatus::DestinationFull)
		LOG_INFO("inventory full", LOG_HERE(here), {"target", target});
	else if (result.status == TransferStatus::Moved) {
		LOG_INFO("player took an item", LOG_HERE(here), {"item", result.id});
		if (schedule(respawn))
			if (Item* taken = here.player->inventory().findItem(result.id))
				taken->clearOrigin();
	}
	return result;
}

TransferResult Game::drop(long long player_id, const std::string& target) {
	Presence here = locate(player_id, "drop");
	if (!here)
		return {TransferStatus::NotFound, ""};

	TransferResult result = transfer(here.player->inventory(), here.room->inventory(), target);
	if (result.status == TransferStatus::NotFound)
		LOG_DEBUG("no such item to drop", LOG_HERE(here), {"target", target});
	else if (result.status == TransferStatus::Moved)
		LOG_INFO("player dropped an item", LOG_HERE(here), {"item", result.id});
	return result;
}

static const char* actionName(CombatAction action) {
	switch (action) {
		case CombatAction::ATTACK:	return "attack";
		case CombatAction::DEFEND:	return "defend";
		case CombatAction::FLEE:	return "flee";
	}
	return "";
}

bool Game::inCombat(long long player_id) const {
	return _combats.find(player_id) != _combats.end();
}

CombatReport Game::attack(long long player_id, const std::string& target) {
	Presence here = locate(player_id, "attack");
	if (!here)
		return {CombatStatus::NOT_FOUND};

	auto fight = _combats.find(player_id);
	if (fight == _combats.end()) {
		Npc* npc = here.room->findNpc(target);
		if (!npc) {
			LOG_DEBUG("no such NPC in the room", LOG_HERE(here), {"target", target});
			return {CombatStatus::NOT_FOUND};
		}
		if (npc->definition().role() != NPC_ROLE_ENEMY) {
			LOG_DEBUG("player tried to attack a non-hostile NPC", LOG_HERE(here), {"npc", npc->id()});
			return {CombatStatus::NOT_HOSTILE};
		}
		fight = _combats.emplace(player_id, startCombat(npc->id())).first;
		LOG_INFO("combat started", LOG_HERE(here), {"npc", npc->id()});
	} else if (!target.empty() && here.room->findNpc(target) != here.room->findNpc(fight->second.enemy_id)) {
		return {CombatStatus::ANOTHER_ENEMY};		// no switching target in the middle of a fight
	}
	return resolveRound(here, fight, CombatAction::ATTACK);
}

CombatReport Game::defend(long long player_id) {
	return continueFight(player_id, CombatAction::DEFEND, "defend");
}

CombatReport Game::flee(long long player_id) {
	return continueFight(player_id, CombatAction::FLEE, "flee");
}

CombatReport Game::continueFight(long long player_id, CombatAction action, const char* name) {
	Presence here = locate(player_id, name);
	if (!here)
		return {CombatStatus::NOT_FOUND};
	auto fight = _combats.find(player_id);
	if (fight == _combats.end())
		return {CombatStatus::NOT_IN_COMBAT};
	return resolveRound(here, fight, action);
}

CombatReport Game::resolveRound(Presence& here, std::unordered_map<long long, Combat>::iterator fight, CombatAction action) {
	const long long player_id = here.player->id();
	Npc* npc = here.room->findNpc(fight->second.enemy_id);
	if (!npc) {								// killed by someone else in the meantime
		here.player->stats().in_combat = false;
		_combats.erase(fight);
		return {CombatStatus::NOT_FOUND};
	}

	CombatReport report;
	report.round = fight->second.round;
	report.outcome = playRound(fight->second, action, here.player->stats(), npc->stats());
	report.player_hp = here.player->stats().hp;
	report.enemy_hp = npc->stats().hp;
	report.player_status = here.player->stats().status();
	LOG_INFO("combat round", LOG_HERE(here), {"npc", npc->id()}, {"round", report.round}, {"action", actionName(action)},
		{"player_hp", report.player_hp}, {"enemy_hp", report.enemy_hp});

	// the end of the fight: the fight is erased by its key because teleport erases it too
	if (report.outcome.enemy_dead)
		enemyDied(here, npc);
	else if (report.outcome.player_dead)
		playerDied(here, npc->id());
	else if (report.outcome.escaped)
		playerFled(here);
	if (report.outcome.enemy_dead || report.outcome.player_dead || report.outcome.escaped)
		_combats.erase(player_id);
	return report;
}

void Game::enemyDied(Presence& here, Npc* npc) {
	const std::string npc_id = npc->id();
	const std::vector<std::string> dropped = npc->dropLoot(here.room->inventory(), _world.item_database);
	std::string loot;
	for (const std::string& item : dropped)
		loot += (loot.empty() ? "" : LOOT_SEPARATOR) + item;
	LOG_INFO("npc died", LOG_HERE(here), {"npc", npc_id}, {"loot", loot});
	schedule(Respawn::of(RespawnKind::Npc, *npc));
	here.room->removeNpc(npc);
}

void Game::playerDied(Presence& here, const std::string& enemy_id) {
	LOG_INFO("player died", LOG_HERE(here), {"npc", enemy_id});
	teleport(here.player->id(), _world.respawn_room);
	here.player->stats().revive(world().respawn_life);
	LOG_INFO("player respawned", {"client", here.player->id()}, {"player", here.player->name()}, {"room", _world.respawn_room}, {"hp", here.player->stats().hp});
}

// The player runs away through an exit chosen at random (he stays here if the room has none)
void Game::playerFled(Presence& here) {
	LOG_INFO("player fled", LOG_HERE(here));
	const std::vector<std::string> directions = lookup::sortedIds(here.room->exits());
	if (directions.empty())
		return;
	teleport(here.player->id(), here.room->exits().at(directions[randomIndex(directions.size())]));
}

const Stats* Game::status(long long player_id) const {
	auto player = _players.find(player_id);
	if (player == _players.end()) {
		LOG_WARN("status of an unknown player", {"client", player_id});
		return nullptr;
	}
	return &player->second.stats();
}

bool Game::schedule(const Respawn& respawn) {
	if (respawn.seconds <= 0)
		return false;
	_respawns.emplace(std::chrono::steady_clock::now() + std::chrono::seconds(respawn.seconds), respawn);
	return true;
}

template <typename Db, typename Place>
static void respawnIn(const Db& database, const std::string& respawn_label, const std::string& room_id, const std::string& def_id,
	int seconds, Room& room, Place place) {
	const auto instance = spawnInstance(database, def_id, room_id, seconds, room, place);
	LOG_INFO(respawn_label + " respawned", {"room", room_id}, {respawn_label, instance.id()});
}

void Game::tick(std::chrono::steady_clock::time_point now) {
	while (!_respawns.empty() && _respawns.begin()->first <= now) {
		const Respawn respawn = _respawns.begin()->second;
		_respawns.erase(_respawns.begin());

		auto room = _world.rooms.find(respawn.room_id);
		if (room == _world.rooms.end()) {
			LOG_ERROR("respawn in an unknown room", {"room", respawn.room_id}, {"definition", respawn.def_id});
			continue;
		}
		if (respawn.kind == RespawnKind::Npc)
			respawnIn(_world.npc_database, "npc", respawn.room_id, respawn.def_id, respawn.seconds, room->second,
				[](Room& r, const Npc& npc) { r.addNpc(npc); });
		else
			respawnIn(_world.item_database, "item", respawn.room_id, respawn.def_id, respawn.seconds, room->second,
				[](Room& r, const Item& item) { r.inventory().add(item); });
	}
}
