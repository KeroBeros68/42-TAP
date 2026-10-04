#ifndef ITEMDB_HPP
# define ITEMDB_HPP

# include <unordered_map>
# include "itemDef.hpp"

class ItemDb {
	private:
		std::unordered_map<std::string, ItemDef> _items;

	public:
		bool addItem(const ItemDef& item);
		const ItemDef& item(const std::string& id) const;
		const std::unordered_map<std::string, ItemDef>& items() const;
};

#endif