#ifndef PLAYER_HPP
# define PLAYER_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "inventory.hpp"
# include "stats.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- New players --------------------------------------------------------------
# define PLAYER_MAX_HP				100	// hit points of a new player
# define PLAYER_ATTACK				10	// attack of a new player
# define PLAYER_DEFENSE				10	// defense of a new player
# define PLAYER_HIT					10	// accuracy of a new player
# define PLAYER_FLEE				10	// evasion of a new player
# define PLAYER_INVENTORY_CAPACITY	10	// items a player can carry

//##############################################################################
//#                                   PLAYER                                   #
//##############################################################################

class Player {
	private:
		//######################### VARIABLES ##########################
		long long	_id;
		std::string	_name;
		std::string	_current_map;
		Inventory	_inventory;
		Stats		_stats;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Builds a new player, with the stats and the capacity defined by the PLAYER_* values.
		 *
		 * @param id   id of the player (the one of his connection)
		 * @param name name of the player
		 * @param map  id of the room where he starts
		 */
		Player(long long id, const std::string& name, const std::string& map);

		//########################## IDENTITY ##########################
		/** Returns the id of the player. */
		long long id() const;

		/** Returns the name of the player. */
		const std::string& name() const;

		//########################## LOCATION ##########################
		/** Returns the id of the room where the player is. */
		const std::string& currentMap() const;

		/** Records the room where the player is (the rooms themselves are not updated). */
		void setCurrentMap(const std::string& map);

		//########################### STATE ############################
		/** Returns the inventory of the player, to change it. */
		Inventory& inventory();

		/** Returns the inventory of the player. */
		const Inventory& inventory() const;

		/** Returns the combat stats of the player. */
		const Stats& stats() const;

		/** Returns the combat stats of the player, to change them. */
		Stats& stats();
};

#endif
