#ifndef ITEM_HPP
# define ITEM_HPP

# include <string>
# include <unordered_map>

class ItemDef {
	private:
		std::string _id;
		std::string _name;
		std::string _description;
		bool _obtainable;

	public:
		ItemDef(const std::string& id, const std::string& name, const std::string& description, bool obtainable);

		const std::string& id() const;
		const std::string& name() const;
		const std::string& description() const;
		bool isObtainable() const;
};

class Item {
	private:
		static inline int _current_id = 0;
		std::string _id;
		const ItemDef* _definition;

	public:
		Item(const ItemDef& def);

		const std::string& id() const;
		const ItemDef& definition() const;
		const std::string& name() const;
};

class ItemDb {
	private:
		std::unordered_map<std::string, ItemDef> _items;

	public:
		bool addItem(const ItemDef& item);
		const ItemDef& item(const std::string& id) const;
		const std::unordered_map<std::string, ItemDef>& items() const;

		Item instantiate(const std::string& id) const;
};

#endif
