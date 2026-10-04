#include "itemDb.hpp"

bool ItemDb::addItem(const ItemDef& item) {
	return _items.emplace(item.id(), item).second;
}

const ItemDef& ItemDb::item(const std::string& id) const {
	return _items.at(id);
}

const std::unordered_map<std::string, ItemDef>& ItemDb::items() const {
	return _items;
}