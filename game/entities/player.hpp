#ifndef PLAYER_HPP
# define PLAYER_HPP

# include <optional>
# include <string>
# include <unordered_map>

# include "inventory.hpp"

class Player {
	private:
		long long	_id;
		std::string	_name;
		std::string	_current_map;		// id of the room where the player is
		Inventory	_inventory;			// limited to PLAYER_INVENTORY_CAPACITY items
		// later: hp, group, quests...

	public:
		Player(long long id, const std::string& name, const std::string& map);

		long long id() const;
		const std::string& name() const;

		const std::string& currentMap() const;
		void setCurrentMap(const std::string& map);

		Inventory& inventory();
		const Inventory& inventory() const;
};

#endif
