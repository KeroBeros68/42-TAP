#include "game.hpp"
#include "../shared/logger.hpp"

#include <iostream>

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
	auto player = _players.find(player_id);
	if (player == _players.end()) {
		LOG_WARN("look of an unknown player", {"client", player_id});
		return std::nullopt;
	}

	auto room = _world.rooms.find(player->second.currentMap());
	if (room == _world.rooms.end()) {
		LOG_WARN("player is on an unknown room", {"client", player_id}, {"room", player->second.currentMap()});
		return std::nullopt;
	}

	LookData data{&room->second, {}};
	for (long long id : room->second.currentPlayer()) {
		auto other = _players.find(id);
		if (other != _players.end())
			data.players.push_back(other->second.name());
	}
	return data;
}

bool Game::move(long long player_id, const std::string& direction) {
	auto player = _players.find(player_id);
	if (player == _players.end()) {
		LOG_WARN("move of an unknown player", {"client", player_id});
		return false;
	}

	auto origin = _world.rooms.find(player->second.currentMap());
	if (origin == _world.rooms.end()) {
		LOG_WARN("player is on an unknown room", {"client", player_id}, {"room", player->second.currentMap()});
		return false;
	}

	auto exit = origin->second.exits().find(direction);
	if (exit == origin->second.exits().end()) {
		LOG_DEBUG("no exit in this direction", {"client", player_id}, {"player", player->second.name()}, {"room", origin->first}, {"direction", direction});
		return false;
	}

	auto destination = _world.rooms.find(exit->second);
	if (destination == _world.rooms.end())
		return false;

	player->second.stats().in_combat = false;
	origin->second.removePlayer(player_id);
	destination->second.addPlayer(player_id);
	player->second.setCurrentMap(exit->second);
	LOG_INFO("player moved", {"client", player_id}, {"player", player->second.name()}, {"from", origin->first}, {"to", exit->second}, {"direction", direction});
	return true;
}

std::optional<std::string> Game::talk(long long player_id, const std::string& target) const {
	auto player = _players.find(player_id);
	if (player == _players.end()) {
		LOG_WARN("talk of an unknown player", {"client", player_id});
		return std::nullopt;
	}

	auto room = _world.rooms.find(player->second.currentMap());
	if (room == _world.rooms.end()) {
		LOG_WARN("player is on an unknown room", {"client", player_id}, {"room", player->second.currentMap()});
		return std::nullopt;
	}

	const Npc* npc = room->second.findNpc(target);
	if (!npc) {
		LOG_DEBUG("no such NPC in the room", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"target", target});
		return std::nullopt;
	}

	LOG_INFO("player talked to an npc", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"npc", npc->id()});
	return npc->talk();
}

TransferResult Game::take(long long player_id, const std::string& target) {
	auto player = _players.find(player_id);
	if (player == _players.end()) {
		LOG_WARN("take of an unknown player", {"client", player_id});
		return {TransferStatus::NotFound, ""};
	}

	auto room = _world.rooms.find(player->second.currentMap());
	if (room == _world.rooms.end()) {
		LOG_WARN("player is on an unknown room", {"client", player_id}, {"room", player->second.currentMap()});
		return {TransferStatus::NotFound, ""};
	}

	const Item* found = room->second.inventory().findItem(target);
	if (!found || !found->definition().isObtainable()) {
		LOG_DEBUG("no such item to take", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"target", target});
		return {TransferStatus::NotFound, ""};
	}

	TransferResult result = transfer(room->second.inventory(), player->second.inventory(), found->id());
	if (result.status == TransferStatus::DestinationFull)
		LOG_INFO("inventory full", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"target", target});
	else if (result.status == TransferStatus::Moved)
		LOG_INFO("player took an item", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"item", result.id});
	return result;
}

TransferResult Game::drop(long long player_id, const std::string& target) {
	auto player = _players.find(player_id);
	if (player == _players.end()) {
		LOG_WARN("drop of an unknown player", {"client", player_id});
		return {TransferStatus::NotFound, ""};
	}

	auto room = _world.rooms.find(player->second.currentMap());
	if (room == _world.rooms.end()) {
		LOG_WARN("player is on an unknown room", {"client", player_id}, {"room", player->second.currentMap()});
		return {TransferStatus::NotFound, ""};
	}

	TransferResult result = transfer(player->second.inventory(), room->second.inventory(), target);
	if (result.status == TransferStatus::NotFound)
		LOG_DEBUG("no such item to drop", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"target", target});
	else if (result.status == TransferStatus::Moved)
		LOG_INFO("player dropped an item", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"item", result.id});
	return result;
}

CombatResult Game::attack(long long player_id, const std::string& target) {
	auto player = _players.find(player_id);
	if (player == _players.end()) {
		LOG_WARN("attack of an unknown player", {"client", player_id});
		return {AttackStatus::NOT_FOUND, 0, 0, 0, 0, false, false};
	}

	auto room = _world.rooms.find(player->second.currentMap());
	if (room == _world.rooms.end()) {
		LOG_WARN("player is on an unknown room", {"client", player_id}, {"room", player->second.currentMap()});
		return {AttackStatus::NOT_FOUND, 0, 0, 0, 0, false, false};
	}

	Npc* npc = room->second.findNpc(target);
	if (!npc) {
		LOG_DEBUG("no such NPC in the room", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"target", target});
		return {AttackStatus::NOT_FOUND, 0, 0, 0, 0, false, false};
	}

	if (npc->definition().role() != NPC_ROLE_ENEMY) {
		LOG_DEBUG("player tried to attack a non-hostile NPC", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"npc", npc->id()});
		return {AttackStatus::NOT_HOSTILE, 0, 0, 0, 0, false, false};
	}

	LOG_INFO("player attacked an npc", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"npc", npc->id()});

	CombatResult result = fight(player->second.stats(), npc->stats());

	if (result.defender_dead) {
		const std::string npc_id = npc->id();
		const std::vector<std::string> dropped = npc->dropLoot(room->second.inventory(), _world.item_database);
		std::string loot;
		for (const std::string& item : dropped)
			loot += (loot.empty() ? "" : ",") + item;
		LOG_INFO("npc died", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"npc", npc_id}, {"loot", loot});
		room->second.removeNpc(npc);
	}

	if (result.attacker_dead) {
		LOG_INFO("player died", {"client", player_id}, {"player", player->second.name()}, {"room", room->first}, {"npc", npc->id()});
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
