//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <iostream>
# include <utility>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "game.hpp"
# include "../shared/logger.hpp"
# include "utils/lookup.hpp"
# include "world/spawn.hpp"

//##############################################################################
//#                                    GAME                                    #
//##############################################################################

//######################## CONSTRUCTOR #########################

Game::Game() : _combat(*this) {}

//########################## PRESENCE ##########################

ConstPresence Game::locate(long long player_id, const char* action) const {
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

Presence Game::locate(long long player_id, const char* action) {
	const ConstPresence here = std::as_const(*this).locate(player_id, action);
	return {const_cast<Player*>(here.player), const_cast<Room*>(here.room)};
}

Player* Game::findPlayer(long long player_id) {
	const auto player = _players.find(player_id);
	return player == _players.end() ? nullptr : &player->second;
}

Room* Game::findRoom(const std::string& room_id) {
	const auto room = _world.rooms.find(room_id);
	return room == _world.rooms.end() ? nullptr : &room->second;
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
	emitPresence(*here.room, EVT_PRESENCE_LEAVE, here.player->name());
	emitPresence(destination->second, EVT_PRESENCE_ENTER, here.player->name());
	destination->second.addPlayer(player_id);
	here.player->setCurrentMap(target);
	return true;
}

//########################### EVENTS ###########################

EventBus& Game::events() {
	return _events;
}

void Game::emitPresence(const Room& room, const char* type, const std::string& name) {
	_events.toRoom(room, eventLine(EVT_ROOM, std::string(type) + CMD_SEPARATOR + name));
}

void Game::emitPlayerCount() {
	_events.send(playerIds(), eventLine(EVT_STATS, EVT_PLAYERS_FIELD + std::to_string(_players.size())));
}

void Game::emitSpawn(const Room& room, const char* kind, const std::string& id) {
	_events.toRoom(room, eventLine(EVT_ROOM, std::string(EVT_SPAWN) + CMD_SEPARATOR + kind + CMD_SEPARATOR + id));
}

//########################## RESPAWN ###########################

void Game::enemyDied(Room& room, Npc* npc, const std::string& killer) {
	const std::string npc_id = npc->id();
	const std::vector<std::string> dropped = npc->dropLoot(room.inventory(), _world.item_database);
	std::string loot;
	for (const std::string& item : dropped)
		loot += (loot.empty() ? "" : LOOT_SEPARATOR) + item;
	LOG_INFO("npc died", {"room", room.id()}, {"player", killer}, {"npc", npc_id}, {"loot", loot});
	_respawns.schedule(Respawn::of(RespawnKind::Npc, *npc));
	room.removeNpc(npc);
}

const std::string& Game::respawnRoom() const {
	return _world.respawn_room;
}

int Game::respawnLife() const {
	return _world.respawn_life;
}

//########################## PLAYERS ###########################

void Game::addPlayer(long long id, const std::string& name) {
	Player player(id, name, _world.start_room);
	_players.insert_or_assign(id, player);

	auto room = _world.rooms.find(player.currentMap());
	if (room == _world.rooms.end()) {
		LOG_WARN("unknown map for player", {"client", id}, {"room", player.currentMap()});
		return;
	}
	emitPresence(room->second, EVT_PRESENCE_ENTER, player.name());
	room->second.addPlayer(id);
	LOG_INFO("player entered room", {"client", id}, {"player", player.name()}, {"room", player.currentMap()});
	emitPlayerCount();
}

void Game::removePlayer(long long id) {
	auto player = _players.find(id);
	if (player == _players.end())
		return;

	auto room = _world.rooms.find(player->second.currentMap());
	if (room != _world.rooms.end()) {
		room->second.removePlayer(id);
		emitPresence(room->second, EVT_PRESENCE_LEAVE, player->second.name());
	}
	_combat.abandon(id);
	LOG_INFO("player left room", {"client", id}, {"player", player->second.name()}, {"room", player->second.currentMap()});
	_players.erase(player);
	emitPlayerCount();
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

const Stats* Game::status(long long player_id) const {
	auto player = _players.find(player_id);
	if (player == _players.end()) {
		LOG_WARN("status of an unknown player", {"client", player_id});
		return nullptr;
	}
	return &player->second.stats();
}

//########################### WORLD ############################

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

//########################### ITEMS ############################

std::optional<std::vector<std::string>> Game::showInventories(long long player_id) const {
	ConstPresence here = locate(player_id, "inventory");
	if (!here)
		return std::nullopt;
	return here.player->inventory().ids();
}

TransferResult Game::takeItem(long long player_id, const std::string& target) {
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
		if (_respawns.schedule(respawn))
			if (Item* taken = here.player->inventory().findItem(result.id))
				taken->clearOrigin();
	}
	return result;
}

TransferResult Game::dropItem(long long player_id, const std::string& target) {
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

//########################### COMBAT ###########################

CombatReport Game::attack(long long player_id, const std::string& target) {
	return _combat.attack(player_id, target);
}

CombatReport Game::defend(long long player_id) {
	return _combat.defend(player_id);
}

CombatReport Game::flee(long long player_id) {
	return _combat.flee(player_id);
}

bool Game::inCombat(long long player_id) const {
	return _combat.inCombat(player_id);
}

std::optional<CombatView> Game::combatView(long long player_id) const {
	return _combat.view(player_id);
}

//############################ TIME ############################

void Game::tick(std::chrono::steady_clock::time_point now) {
	_combat.update(now);
	_respawns.update(now, _world, [this](const Room& room, const char* kind, const std::string& id) {
		emitSpawn(room, kind, id);
	});
}

std::vector<Event> Game::takeEvents() {
	return _events.take();
}
