#ifndef NPC_HPP
# define NPC_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "definition.hpp"
# include "instance.hpp"
# include "inventory.hpp"
# include "respawnable.hpp"
# include "stats.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Roles --------------------------------------------------------------------
# define NPC_ROLE_ENEMY "enemy"	// role of the npcs that can be attacked

//##############################################################################
//#                                  NPC DEF                                   #
//##############################################################################

class NpcDef : public Definition {
	private:
		//######################### VARIABLES ##########################
		std::string _role;
		std::vector<std::string> _dialogues;
		Stats _stats;
		std::vector<std::string> _loots;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Builds the definition of an npc.
		 *
		 * @param id          id of the definition
		 * @param name        name shown to the players
		 * @param role        role of the npc, like NPC_ROLE_ENEMY
		 * @param description description shown to the players
		 * @param dialogues   the lines the npc says
		 * @param stats       combat stats (only for an enemy)
		 * @param loots       ids of the items it drops when it dies
		 */
		NpcDef(const std::string& id, const std::string& name, const std::string& role,
			const std::string& description, const std::vector<std::string>& dialogues,
			const Stats& stats, const std::vector<std::string>& loots);

		//########################## GETTERS ###########################
		/** Returns the role of the npc. */
		const std::string& role() const;

		/**
		 * Picks one of the lines the npc says, at random.
		 *
		 * @return the line (an empty text if the npc has none)
		 */
		const std::string& dialogue() const;

		/** Returns the ids of the items the npc drops when it dies. */
		const std::vector<std::string>& loots() const;

		/** Returns the combat stats the npc starts with. */
		const Stats& stats() const;
};

//##############################################################################
//#                                    NPC                                     #
//##############################################################################

class Npc : public Instance<NpcDef>, public Respawnable {
	private:
		//######################### VARIABLES ##########################
		Stats _stats;

	public:
		//######################## CONSTRUCTOR #########################
		/** Creates an npc from its definition, with the combat stats of the definition. */
		Npc(const NpcDef& def);

		//######################### FUNCTIONS ##########################
		/**
		 * Picks one of the lines the npc says, at random.
		 *
		 * @return the line (an empty text if the npc has none)
		 */
		const std::string& talk() const;

		/** Returns the current combat stats of the npc. */
		const Stats& stats() const;

		/** Returns the current combat stats of the npc, to change them. */
		Stats& stats();

		/**
		 * Creates the items of its loot on the floor.
		 *
		 * @param floor inventory of the room where the npc is
		 * @param items the items, to create the ones of the loot
		 * @return      the ids of the items that were created
		 */
		std::vector<std::string> dropLoot(Inventory& floor, const ItemDb& items) const;
};

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

typedef Database<NpcDef, Npc> NpcDb;

#endif
