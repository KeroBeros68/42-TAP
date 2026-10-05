#ifndef WORLD_HPP
# define WORLD_HPP

# include <string>
# include <vector>

# include "item.hpp"
# include "npc.hpp"
# include "room.hpp"

class World {
	private:
		void loadWorld();

	public:
		RoomMap		rooms;
		std::string	start_room;
		std::string	respawn_room;
		NpcDb		npc_database;
		ItemDb		item_database;

		World();

		std::string look(const std::string& room_id, const std::vector<std::string>& players) const;

};

#endif
