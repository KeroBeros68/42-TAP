#include "room.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "../shared/defines.hpp"

Room::Room(const std::string& id, const std::string& name, const std::string& description,
	const std::unordered_map<std::string, std::string>& exits,
	const std::vector<Spawn>& item_spawns, const std::vector<Spawn>& npc_spawns
)
	: _id(id), _name(name), _description(description), _exits(exits), _item_spawns(item_spawns), _npc_spawns(npc_spawns) {}

const std::string& Room::id() const {
	return _id;
}

const std::string& Room::name() const {
	return _name;
}

const std::string& Room::description() const {
	return _description;
}

const std::unordered_map<std::string, std::string>& Room::exits() const {
	return _exits;
}

const std::vector<Spawn>& Room::itemSpawns() const {
	return _item_spawns;
}

const std::vector<Spawn>& Room::npcSpawns() const {
	return _npc_spawns;
}

std::vector<std::string> Room::npcs() const {
	std::vector<std::string> ids;
	for (const auto& [id, npc] : _alive_npcs)
		ids.push_back(id);
	std::sort(ids.begin(), ids.end());
	return ids;
}

std::vector<std::string> Room::items() const {
	std::vector<std::string> ids;
	for (const auto& [id, npc] : _alive_items)
		ids.push_back(id);
	std::sort(ids.begin(), ids.end());
	return ids;
}

const std::unordered_map<std::string, Npc>& Room::aliveNpcs() const {
	return _alive_npcs;
}

const std::unordered_map<std::string, Item>& Room::aliveItems() const {
	return _alive_items;
}

void Room::addItem(const Item& item) {
	_alive_items.emplace(item.id(), item);
}

void Room::addNpc(const Npc& npc) {
	_alive_npcs.emplace(npc.id(), npc);
}

static std::string lowerCase(const std::string& text) {
	std::string lower = text;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
	return lower;
}

static bool designates(const Npc& npc, const std::string& query) {
	const std::string definition_id = lowerCase(npc.definition().id());
	const size_t dot = definition_id.find('.');
	const std::string short_id = (dot == std::string::npos) ? definition_id : definition_id.substr(dot + 1);	// "guard"
	const std::string name = lowerCase(npc.name());

	if (query == lowerCase(npc.id()) || query == definition_id || query == short_id || query == name)
		return true;

	std::istringstream words(name);
	std::string word;
	while (words >> word) {
		if (word.back() == ',')
			word.pop_back();
		if (word == query)
			return true;
	}
	return false;
}

const Npc* Room::findNpc(const std::string& query) const {
	const size_t first = query.find_first_not_of(' ');
	if (first == std::string::npos)
		return nullptr;
	const std::string wanted = lowerCase(query.substr(first, query.find_last_not_of(' ') - first + 1));

	for (const std::string& id : npcs()) {
		const Npc& npc = _alive_npcs.at(id);
		if (designates(npc, wanted))
			return &npc;
	}
	return nullptr;
}

const std::vector<long long>& Room::currentPlayer() const {
	return _current_player;
}

void Room::addPlayer(long long id) {
	if (std::find(_current_player.begin(), _current_player.end(), id) == _current_player.end())
		_current_player.push_back(id);
}

void Room::removePlayer(long long id) {
	_current_player.erase(std::remove(_current_player.begin(), _current_player.end(), id), _current_player.end());
}

std::string oppositeDirection(const std::string& direction) {
	if (direction == DIR_NORTH)
		return DIR_SOUTH;
	if (direction == DIR_SOUTH)
		return DIR_NORTH;
	if (direction == DIR_EAST)
		return DIR_WEST;
	if (direction == DIR_WEST)
		return DIR_EAST;
	return "";
}
