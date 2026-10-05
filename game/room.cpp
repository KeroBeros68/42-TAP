#include "room.hpp"

#include <algorithm>

#include "../shared/defines.hpp"

Room::Room(const std::string& id, const std::string& name, const std::string& description,
	const std::unordered_map<std::string, std::string>& exits, const std::vector<std::string>& items)
	: _id(id), _name(name), _description(description), _exits(exits), _items(items) {}

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

const std::vector<std::string>& Room::items() const {
	return _items;
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
