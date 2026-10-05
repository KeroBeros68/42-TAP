#include "item.hpp"

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
