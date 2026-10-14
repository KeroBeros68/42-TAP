#include "item.hpp"
#include "../../shared/defines.hpp"

ItemDef::ItemDef(const std::string& id, const std::string& name, const std::string& description, bool obtainable) 
	: _id(id), _name(name), _description(description), _obtainable(obtainable) {}

const std::string& ItemDef::id() const {
	return _id;
}

const std::string& ItemDef::name() const {
	return _name;
}

const std::string& ItemDef::description() const {
	return _description;
}

bool ItemDef::isObtainable() const {
	return _obtainable;
}

bool ItemDb::addItem(const ItemDef& item) {
	return _items.emplace(item.id(), item).second;
}

const ItemDef& ItemDb::item(const std::string& id) const {
	return _items.at(id);
}

const std::unordered_map<std::string, ItemDef>& ItemDb::items() const {
	return _items;
}

void Item::setOrigin(const std::string& room, int respawn_seconds) {
	_home_room = room;
	_respawn_seconds = respawn_seconds;
}

void Item::clearOrigin() {
	_home_room.clear();
	_respawn_seconds = 0;
}

const std::string& Item::homeRoom() const {
	return _home_room;
}

int Item::respawnSeconds() const {
	return _respawn_seconds;
}

Item ItemDb::instantiate(const std::string& id) const {
	return Item(item(id));
}

Item::Item(const ItemDef& def)
	: _id(def.id() + INSTANCE_SEPARATOR + std::to_string(++_current_id)), _definition(&def) {}

const std::string& Item::id() const {
	return _id;
}

const ItemDef& Item::definition() const {
	return *_definition;
}

const std::string& Item::name() const {
	return _definition->name();
}