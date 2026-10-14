#ifndef WORLD_HPP
# define WORLD_HPP

# include <string>
# include <vector>

# include "../entities/item.hpp"
# include "../entities/npc.hpp"
# include "room.hpp"

# define DATA_DIR_RELATIVE	"/game/ressources/"
# define NPC_FILE			"npc.json"
# define ITEM_FILE			"items.json"
# define ROOM_FILE			"room.json"

class World {
	private:
		void loadWorld();

	public:
		RoomMap		rooms;
		std::string	start_room;
		std::string	respawn_room;
		int			respawn_life;
		NpcDb		npc_database;
		ItemDb		item_database;

		World();

};

#endif
