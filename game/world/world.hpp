#ifndef WORLD_HPP
# define WORLD_HPP

# include <string>
# include <vector>

# include "../entities/item.hpp"
# include "../entities/npc.hpp"
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

};

#endif
