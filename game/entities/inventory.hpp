#ifndef INVENTORY_HPP
# define INVENTORY_HPP

# include <optional>
# include <string>
# include <unordered_map>
# include <vector>

# include "item.hpp"

class Inventory {
	private:
		std::unordered_map<std::string, Item> _items;
		size_t _capacity;

	public:
		explicit Inventory(size_t capacity = 0);

		const Item* findItem(const std::string& query) const;
		bool isFull() const;

		bool add(const Item& item);
		std::optional<Item> remove(const std::string& id);

		std::vector<std::string> ids() const;
		const std::unordered_map<std::string, Item>& items() const;
		size_t size() const;
};

enum class TransferStatus { Moved, NotFound, DestinationFull };

struct TransferResult {
	TransferStatus	status;
	std::string		id;
};

TransferResult transfer(Inventory& from, Inventory& to, const std::string& query);

#endif
