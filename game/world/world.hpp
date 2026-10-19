#ifndef WORLD_HPP
# define WORLD_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../entities/item.hpp"
# include "../entities/npc.hpp"
# include "room.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Data files ---------------------------------------------------------------
# define DATA_DIR_RELATIVE	"/game/ressources/"	// folder of the data files, from the root of the project
# define NPC_FILE			"npc.json"			// file of the npcs
# define ITEM_FILE			"items.json"		// file of the items
# define ROOM_FILE			"room.json"			// file of the rooms

//##############################################################################
//#                                   WORLD                                    #
//##############################################################################

class World {
	private:
		//######################### FUNCTIONS ##########################
		/** Reads and checks the data files, then creates the npcs and the items of the spawns. */
		void loadWorld();

	public:
		//######################### VARIABLES ##########################
		RoomMap		rooms;
		std::string	start_room;
		std::string	respawn_room;
		int			respawn_life;
		NpcDb		npc_database;
		ItemDb		item_database;

		//######################## CONSTRUCTOR #########################
		/** Loads the world from the data files (std::runtime_error is thrown if they are invalid). */
		World();
};

#endif
