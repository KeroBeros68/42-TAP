#include "game.hpp"
#include "../shared/logger.hpp"

#include <iostream>
#include <utility>

Game::Game() {}

void Game::addPlayer(long long id, const std::string& name) {
	// a new player starts on the start room of the world
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
	if (room != _world.rooms.end())
		room->second.removePlayer(id);
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
		LOG_DEBUG("no exit in this direction", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"direction", direction});
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
		LOG_DEBUG("no such NPC in the room", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"target", target});
		return std::nullopt;
	}

	LOG_INFO("player talked to an npc", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"npc", npc->id()});
	return npc->talk();
}

TransferResult Game::take(long long player_id, const std::string& target) {
	Presence here = locate(player_id, "take");
	if (!here)
		return {TransferStatus::NotFound, ""};

	const Item* found = here.room->inventory().findItem(target);
	if (!found || !found->definition().isObtainable()) {
		LOG_DEBUG("no such item to take", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"target", target});
		return {TransferStatus::NotFound, ""};
	}

	const std::string item_def = found->definition().id();
	const std::string home_room = found->homeRoom();
	const int respawn_seconds = found->respawnSeconds();

	TransferResult result = transfer(here.room->inventory(), here.player->inventory(), found->id());
	if (result.status == TransferStatus::DestinationFull)
		LOG_INFO("inventory full", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"target", target});
	else if (result.status == TransferStatus::Moved) {
		LOG_INFO("player took an item", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"item", result.id});
		if (respawn_seconds > 0) {
			_respawns.emplace(std::chrono::steady_clock::now() + std::chrono::seconds(respawn_seconds),
				Respawn{RespawnKind::Item, item_def, home_room, respawn_seconds});
			if (Item* taken = here.player->inventory().findItem(result.id))
				taken->clearOrigin();
		}
	}
	return result;
}

TransferResult Game::drop(long long player_id, const std::string& target) {
	Presence here = locate(player_id, "drop");
	if (!here)
		return {TransferStatus::NotFound, ""};

	TransferResult result = transfer(here.player->inventory(), here.room->inventory(), target);
	if (result.status == TransferStatus::NotFound)
		LOG_DEBUG("no such item to drop", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"target", target});
	else if (result.status == TransferStatus::Moved)
		LOG_INFO("player dropped an item", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"item", result.id});
	return result;
}

CombatResult Game::attack(long long player_id, const std::string& target) {
	Presence here = locate(player_id, "attack");
	if (!here)
		return {AttackStatus::NOT_FOUND, 0, 0, 0, 0, false, false};

	Npc* npc = here.room->findNpc(target);
	if (!npc) {
		LOG_DEBUG("no such NPC in the room", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"target", target});
		return {AttackStatus::NOT_FOUND, 0, 0, 0, 0, false, false};
	}

	if (npc->definition().role() != NPC_ROLE_ENEMY) {
		LOG_DEBUG("player tried to attack a non-hostile NPC", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"npc", npc->id()});
		return {AttackStatus::NOT_HOSTILE, 0, 0, 0, 0, false, false};
	}

	LOG_INFO("player attacked an npc", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"npc", npc->id()});

	CombatResult result = fight(here.player->stats(), npc->stats());

	if (result.defender_dead) {
		const std::string npc_id = npc->id();
		const std::vector<std::string> dropped = npc->dropLoot(here.room->inventory(), _world.item_database);
		std::string loot;
		for (const std::string& item : dropped)
			loot += (loot.empty() ? "" : LOOT_SEPARATOR) + item;
		LOG_INFO("npc died", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"npc", npc_id}, {"loot", loot});
		if (npc->respawnSeconds() > 0)
			_respawns.emplace(std::chrono::steady_clock::now() + std::chrono::seconds(npc->respawnSeconds()),
				Respawn{RespawnKind::Npc, npc->definition().id(), npc->homeRoom(), npc->respawnSeconds()});
		here.room->removeNpc(npc);
	}

	if (result.attacker_dead) {
		LOG_INFO("player died", {"client", player_id}, {"player", here.player->name()}, {"room", here.room->id()}, {"npc", npc->id()});
		teleport(player_id, _world.respawn_room);
		here.player->stats().revive(world().respawn_life);
		LOG_INFO("player respawned", {"client", player_id}, {"player", here.player->name()}, {"room", _world.respawn_room}, {"hp", here.player->stats().hp});
	}

	return result;
}

const Stats* Game::status(long long player_id) const {
	auto player = _players.find(player_id);
	if (player == _players.end()) {
		LOG_WARN("status of an unknown player", {"client", player_id});
		return nullptr;
	}
	return &player->second.stats();
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
		if (respawn.kind == RespawnKind::Npc) {
			Npc npc = _world.npc_database.instantiate(respawn.def_id);
			npc.setOrigin(respawn.room_id, respawn.seconds);
			room->second.addNpc(npc);
			LOG_INFO("npc respawned", {"room", respawn.room_id}, {"npc", npc.id()});
		} else {
			Item item = _world.item_database.instantiate(respawn.def_id);
			item.setOrigin(respawn.room_id, respawn.seconds);
			room->second.inventory().add(item);
			LOG_INFO("item respawned", {"room", respawn.room_id}, {"item", item.id()});
		}
	}
}
