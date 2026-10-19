//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "inventory.hpp"
# include "../utils/lookup.hpp"

//##############################################################################
//#                                 INVENTORY                                  #
//##############################################################################

//######################## CONSTRUCTOR #########################

Inventory::Inventory(size_t capacity) : _capacity(capacity) {}

//########################## QUERIES ###########################

const Item* Inventory::findItem(const std::string& query) const {
	return lookup::findIn(_items, query);
}

Item* Inventory::findItem(const std::string& query) {
	return lookup::findIn(_items, query);
}

bool Inventory::isFull() const {
	return _capacity != 0 && _items.size() >= _capacity;
}

std::vector<std::string> Inventory::ids() const {
	return lookup::sortedIds(_items);
}

const std::unordered_map<std::string, Item>& Inventory::items() const {
	return _items;
}

size_t Inventory::size() const {
	return _items.size();
}

//########################## CHANGES ###########################

bool Inventory::add(const Item& item) {
	if (isFull())
		return false;
	return _items.emplace(item.id(), item).second;
}

std::optional<Item> Inventory::remove(const std::string& id) {
	auto it = _items.find(id);
	if (it == _items.end())
		return std::nullopt;
	Item item = it->second;
	_items.erase(it);
	return item;
}

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

TransferResult transfer(Inventory& from, Inventory& to, const std::string& query) {
	const Item* found = from.findItem(query);
	if (!found)
		return {TransferStatus::NotFound, ""};
	if (to.isFull())
		return {TransferStatus::DestinationFull, ""};

	const std::string id = found->id();
	std::optional<Item> item = from.remove(id);
	to.add(*item);
	return {TransferStatus::Moved, id};
}
