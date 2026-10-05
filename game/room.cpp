#include "room.hpp"
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
