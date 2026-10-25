//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <algorithm>
# include <iostream>
# include <utility>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "game.hpp"
# include "../shared/logging/logger.hpp"
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
	const std::string definition_id = id.substr(0, id.find(INSTANCE_SEPARATOR));
	_events.toRoom(room, eventLine(EVT_ROOM, std::string(EVT_SPAWN) + CMD_SEPARATOR + kind + CMD_SEPARATOR + definition_id));
}

//############################ QUESTS ##########################

int Game::questCurrent(const Player& player, const QuestProgress& quest) const {
	const QuestDef& def = quest.definition();
	if (quest.state() == QuestState::COMPLETED)
		return def.count();
	if (def.type() == QuestType::DEFEAT)
		return quest.kills();

	int carried = 0;
	for (const auto& [id, item] : player.inventory().items()) {
		if (item.definition().id() == def.target())
			++carried;
	}
	return std::min(carried, def.count());
}

QuestStatus Game::completeQuest(Player& player, QuestProgress& quest) {
	const QuestDef& def = quest.definition();
	Inventory trial = player.inventory();
	if (def.type() != QuestType::DEFEAT) {
		int to_take = def.count();
		for (const std::string& id : player.inventory().ids()) {
			const Item* item = player.inventory().findItem(id);
			if (to_take > 0 && item->definition().id() == def.target()) {
				trial.remove(id);
				--to_take;
			}
		}
	}
	for (const std::string& reward : def.reward().items) {
		if (!trial.add(_world.item_database.instantiate(reward)))
			return QuestStatus::INVENTORY_FULL;
	}

	player.inventory() = trial;
	player.stats().heal(def.reward().heal);
	quest.complete();
	return QuestStatus::COMPLETED;
}

//########################## EFFECTS ###########################

int Game::regen(long long player_id, int amount) {
	Player* player = findPlayer(player_id);
	if (!player || player->stats().in_combat)
		return 0;

	const int before = player->stats().hp;
	player->stats().heal(amount);
	return player->stats().hp - before;
}

void Game::emitRegen(long long player_id, int gained) {
	_events.send({player_id}, eventLine(EVT_ROOM, std::string(EVT_EFFECT) + CMD_SEPARATOR + EVT_EFFECT_HEALING + CMD_SEPARATOR + std::to_string(gained)));
}

void Game::applyRoomEffects(std::chrono::steady_clock::time_point now) {
	for (const auto& [room_id, room] : _world.rooms) {
		if (!room.effect())
			continue;

		const auto interval = std::chrono::seconds(room.effect()->interval_seconds);
		auto due = _effect_due.find(room_id);
		if (due == _effect_due.end()) {
			_effect_due[room_id] = now + interval;
			continue;
		}
		if (now < due->second)
			continue;
		due->second = now + interval;

		for (long long player_id : room.currentPlayer()) {
			const int gained = regen(player_id, room.effect()->amount);
			if (gained > 0) {
				emitRegen(player_id, gained);
				LOG_INFO("room healed a player", {"client", player_id}, {"room", room_id}, {"hp", gained});
			}
		}
	}
}

//############################ GROUP ###########################

void Game::emitGroup(const std::vector<long long>& players_ids, const char* type, const std::string& data) {
	_events.send(players_ids, eventLine(EVT_GROUP, std::string(type) + CMD_SEPARATOR + data));
}

GroupResult Game::leaveGroup(long long player_id) {
	GroupResult result = _groups.remove(player_id);
	auto player = _players.find(player_id);
	if (result.status == GroupStatus::DONE && player != _players.end()) {
		emitGroup(result.others, EVT_GROUP_LEAVE, player->second.name());
		LOG_INFO("player left group", {"client", player_id}, {"player", player->second.name()}, {"group", result.group_id});
	}
	return result;
}

//########################## RESPAWN ###########################

void Game::enemyDied(Room& room, Npc* npc, const std::string& killer, const std::vector<long long>& participants) {
	const std::string npc_id = npc->id();
	for (long long player_id : participants) {
		for (const QuestProgress* quest : _quests.recordKill(player_id, npc->definition().id()))
			LOG_INFO("quest progress", {"client", player_id}, {"quest", quest->definition().id()}, {"kills", quest->kills()}, {"count", quest->definition().count()});
	}
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
	leaveGroup(id);
	_quests.remove(id);
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
	return here.player->inventory().definitionIds();
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

UseResult Game::useItem(long long player_id, const std::string& target) {
	Presence here = locate(player_id, "use");
	if (!here)
		return {UseStatus::UNKNOWN_PLAYER, "", 0};

	const Item* found = here.player->inventory().findItem(target);
	if (!found) {
		LOG_DEBUG("no such item to use", LOG_HERE(here), {"target", target});
		return {UseStatus::NOT_FOUND, "", 0};
	}
	const std::optional<ItemEffect>& effect = found->definition().effect();
	if (!effect) {
		LOG_DEBUG("item has no effect", LOG_HERE(here), {"item", found->id()});
		return {UseStatus::NOT_USABLE, found->id(), 0};
	}

	const std::string id = found->id();
	const int before = here.player->stats().hp;
	here.player->stats().heal(effect->amount);
	const int healed = here.player->stats().hp - before;
	here.player->inventory().remove(id);
	LOG_INFO("player used an item", LOG_HERE(here), {"item", id}, {"healed", healed});
	return {UseStatus::USED, id, healed};
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

//############################ GROUP ###########################

GroupReply Game::groupCreate(long long player_id) {
	Player* player = findPlayer(player_id);
	if (!player)
		return {GroupStatus::NOT_IN_GROUP, "", true};

	GroupResult result = _groups.create(player_id);
	if (result.status == GroupStatus::DONE)
		LOG_INFO("group created", {"client", player_id}, {"player", player->name()}, {"group", result.group_id});
	return {result.status, result.group_id, false};
}

GroupReply Game::groupInvite(long long player_id, const std::string& target) {
	Player* player = findPlayer(player_id);
	if (!player)
		return {GroupStatus::NOT_IN_GROUP, "", true};

	long long invited_id = 0;
	bool found = false;
	for (const auto& p : _players) {
		if (p.second.name() == target) {
			invited_id = p.first;
			found = true;
			break;
		}
	}
	if (!found) {
		LOG_DEBUG("invitation of an unknown player", {"client", player_id}, {"target", target});
		return {GroupStatus::DONE, "", true};
	}

	GroupResult result = _groups.invite(player_id, invited_id);
	if (result.status == GroupStatus::DONE) {
		emitGroup({invited_id}, EVT_GROUP_INVITE, player->name());
		LOG_INFO("player invited", {"client", player_id}, {"player", player->name()}, {"invited", target}, {"group", result.group_id});
	}
	return {result.status, result.group_id, false};
}

GroupReply Game::groupJoin(long long player_id, const std::string& target) {
	Player* player = findPlayer(player_id);
	if (!player)
		return {GroupStatus::NOT_IN_GROUP, "", true};

	long long member_id = 0;
	bool found = false;
	for (const auto& p : _players) {
		if (p.second.name() == target) {
			member_id = p.first;
			found = true;
			break;
		}
	}
	if (!found) {
		LOG_DEBUG("join of a group of an unknown player", {"client", player_id}, {"target", target});
		return {GroupStatus::DONE, "", true};
	}

	GroupResult result = _groups.join(player_id, member_id);
	if (result.status == GroupStatus::DONE) {
		emitGroup(result.others, EVT_GROUP_JOIN, player->name());
		LOG_INFO("player joined group", {"client", player_id}, {"player", player->name()}, {"group", result.group_id});
	}
	return {result.status, result.group_id, false};
}

GroupReply Game::groupLeave(long long player_id) {
	if (!findPlayer(player_id))
		return {GroupStatus::NOT_IN_GROUP, "", true};

	GroupResult result = leaveGroup(player_id);
	return {result.status, result.group_id, false};
}

//############################ QUESTS ##########################

QuestReply Game::quest(long long player_id, const std::string& target) {
	Presence here = locate(player_id, "quest");
	if (!here)
		return {QuestStatus::UNKNOWN_PLAYER, {}};

	const Npc* npc = here.room->findNpc(target);
	if (!npc) {
		LOG_DEBUG("no such NPC for a quest", LOG_HERE(here), {"target", target});
		return {QuestStatus::NPC_NOT_FOUND, {}};
	}
	const std::string& npc_def = npc->definition().id();

	if (QuestProgress* active = _quests.activeFor(player_id, npc_def)) {
		QuestView view{&active->definition(), active->state(), questCurrent(*here.player, *active)};
		if (view.current < active->definition().count())
			return {QuestStatus::IN_PROGRESS, view};

		const QuestStatus status = completeQuest(*here.player, *active);
		view.state = active->state();
		view.current = questCurrent(*here.player, *active);
		if (status == QuestStatus::COMPLETED)
			LOG_INFO("quest completed", LOG_HERE(here), {"quest", view.def->id()}, {"npc", npc->id()});
		else
			LOG_INFO("quest reward does not fit", LOG_HERE(here), {"quest", view.def->id()});
		return {status, view};
	}

	if (const QuestDef* offered = _quests.offeredBy(_world.quest_database, player_id, npc_def)) {
		QuestProgress& started = _quests.start(player_id, *offered);
		LOG_INFO("quest started", LOG_HERE(here), {"quest", offered->id()}, {"npc", npc->id()});
		return {QuestStatus::STARTED, {offered, started.state(), questCurrent(*here.player, started)}};
	}

	LOG_DEBUG("no quest to give or take", LOG_HERE(here), {"npc", npc->id()});
	return {QuestStatus::NONE, {}};
}

std::optional<std::vector<QuestView>> Game::quests(long long player_id) const {
	ConstPresence here = locate(player_id, "quests");
	if (!here)
		return std::nullopt;

	std::vector<QuestView> views;
	for (const QuestProgress* quest : _quests.list(player_id))
		views.push_back({&quest->definition(), quest->state(), questCurrent(*here.player, *quest)});
	return views;
}

//############################ CHAT ############################

ChatStatus Game::chat(long long player_id, ChatScope scope, const std::string& text) {
	Presence here = locate(player_id, "chat");
	if (!here)
		return ChatStatus::UNKNOWN_PLAYER;

	const std::string data = std::string(EVT_CHAT) + CMD_SEPARATOR + here.player->name() + CMD_SEPARATOR + text;
	if (scope == ChatScope::GLOBAL)
		_events.send(playerIds(), eventLine(EVT_GLOBAL, data));
	else if (scope == ChatScope::ROOM)
		_events.toRoom(*here.room, eventLine(EVT_ROOM, data));
	else {
		const std::vector<long long> members = _groups.membersOf(player_id);
		if (members.empty())
			return ChatStatus::NOT_IN_GROUP;
		_events.send(members, eventLine(EVT_GROUP, data));
	}

	const char* scope_name = scope == ChatScope::GLOBAL ? CHAT_SCOPE_GLOBAL : scope == ChatScope::ROOM ? CHAT_SCOPE_ROOM : CHAT_SCOPE_GROUP;
	LOG_INFO("chat message", LOG_HERE(here), {"scope", scope_name}, {"length", text.size()});
	LOG_DEBUG("chat text", LOG_HERE(here), {"text", text});
	return ChatStatus::SENT;
}

//############################ TIME ############################

void Game::tick(std::chrono::steady_clock::time_point now) {
	_combat.update(now);
	applyRoomEffects(now);
	_respawns.update(now, _world, [this](const Room& room, const char* kind, const std::string& id) {
		emitSpawn(room, kind, id);
	});
}

std::vector<Event> Game::takeEvents() {
	return _events.take();
}
