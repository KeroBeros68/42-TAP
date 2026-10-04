#ifndef WORLD_HPP
# define WORLD_HPP

# include "itemDb.hpp"
# include "npcDb.hpp"
# include "roomDb.hpp"

// The static definitions of the game, loaded once from game/ressources/*.json.
// The JSON reading lives in world.cpp so that the files including this header
// do not have to compile the JSON library.
class World {
	private:
		void loadWorld();

	public:
		RoomDb	room_database;
		NpcDb	npc_database;
		ItemDb	item_database;

		World();
};

#endif
