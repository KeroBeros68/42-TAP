#include "roomDef.hpp"

RoomDef::RoomDef(const std::string& id, const std::string& name, const std::string& description,
	const std::unordered_map<std::string, std::string>& exits, const std::vector<std::string>& items)
	: _id(id), _name(name), _description(description), _exits(exits), _items(items) {}

const std::string& RoomDef::id() const {
	return _id;
}

const std::string& RoomDef::name() const {
	return _name;
}

const std::string& RoomDef::description() const {
	return _description;
}

const std::unordered_map<std::string, std::string>& RoomDef::exits() const {
	return _exits;
}