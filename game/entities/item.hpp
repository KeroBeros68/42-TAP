#ifndef ITEM_HPP
# define ITEM_HPP

# include <string>
# include <unordered_map>

# include "definition.hpp"
# include "respawnable.hpp"
# include "instance.hpp"
class ItemDef : public Definition {
	private:
		bool _obtainable;

	public:
		ItemDef(const std::string& id, const std::string& name, const std::string& description, bool obtainable);

		bool isObtainable() const;
};

class Item : public Instance<ItemDef>, public Respawnable {
	public:
		Item(const ItemDef& def);
};

typedef Database<ItemDef, Item> ItemDb;

#endif
