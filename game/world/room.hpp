#ifndef ROOM_HPP
# define ROOM_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <optional>
# include <string>
# include <unordered_map>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../entities/inventory.hpp"
# include "../entities/npc.hpp"
# include "../utils/lookup.hpp"

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct Spawn {
	std::string	id;
	int			count = 1;
	int			respawn_seconds = 0;
};

//##############################################################################
//#                                    ROOM                                    #
//##############################################################################

class Room {
	private:
		//########################## IDENTITY ##########################
		std::string _id;
		std::string _name;
		std::string _description;

		//########################### EXITS ############################
		std::unordered_map<std::string, std::string> _exits;

		//########################## CONTENT ###########################
		std::vector<Spawn> _item_spawns;
		std::vector<Spawn> _npc_spawns;
		std::unordered_map<std::string, Npc> _alive_npcs;
		Inventory _inventory;

		//########################## PLAYERS ###########################
		std::vector<long long> _current_player;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Builds a room.
		 *
		 * @param id          id of the room
		 * @param name        name shown to the players
		 * @param description description shown to the players
		 * @param exits       id of the room behind each direction
		 * @param item_spawns items created in the room
		 * @param npc_spawns  npcs created in the room
		 */
		Room(const std::string& id, const std::string& name, const std::string& description,
			const std::unordered_map<std::string, std::string>& exits,
			const std::vector<Spawn>& item_spawns, const std::vector<Spawn>& npc_spawns
		);

		//########################## IDENTITY ##########################
		/** Returns the id of the room. */
		const std::string& id() const;

		/** Returns the name of the room. */
		const std::string& name() const;

		/** Returns the description of the room. */
		const std::string& description() const;

		/** Returns the exits: the id of the room behind each direction. */
		const std::unordered_map<std::string, std::string>& exits() const;

		//########################### SPAWNS ###########################
		/** Returns the spawns of the items of the room. */
		const std::vector<Spawn>& itemSpawns() const;

		/** Returns the spawns of the npcs of the room. */
		const std::vector<Spawn>& npcSpawns() const;

		//############################ NPCS ############################
		/** Lists the ids of the npcs that are in the room, in alphabetical order. */
		std::vector<std::string> npcs() const;

		/** Returns the npcs that are in the room, by id. */
		const std::unordered_map<std::string, Npc>& aliveNpcs() const;

		/**
		 * Finds the npc that a query designates.
		 *
		 * @param query instance id, definition id, name or one word of the name, in any case
		 * @return      the npc (the first by id if several match), nullptr if there is none
		 */
		const Npc* findNpc(const std::string& query) const;

		/**
		 * Same as the const version, but gives an npc that can be modified.
		 *
		 * @param query instance id, definition id, name or one word of the name, in any case
		 * @return      the npc (the first by id if several match), nullptr if there is none
		 */
		Npc* findNpc(const std::string& query);

		/**
		 * Puts an npc in the room.
		 *
		 * @param npc the npc to add
		 */
		void addNpc(const Npc& npc);

		/**
		 * Takes an npc out of the room: it is destroyed.
		 *
		 * @param npc the npc, which is not valid afterwards
		 */
		void removeNpc(Npc* npc);

		//########################### ITEMS ############################
		/** Lists the ids of the items on the floor, in alphabetical order. */
		std::vector<std::string> items() const;

		/** Returns the floor of the room, to change it. */
		Inventory& inventory();

		/** Returns the floor of the room. */
		const Inventory& inventory() const;

		//########################## PLAYERS ###########################
		/** Returns the ids of the players who are in the room. */
		const std::vector<long long>& currentPlayer() const;

		/** Records that a player entered the room. */
		void addPlayer(long long id);

		/** Records that a player left the room. */
		void removePlayer(long long id);
};

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

using RoomMap = std::unordered_map<std::string, Room>;

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Finds the direction that leads back.
 *
 * @param direction north, east, south or west
 * @return          the opposite direction (an empty text if it is not a direction)
 */
std::string oppositeDirection(const std::string& direction);

#endif
