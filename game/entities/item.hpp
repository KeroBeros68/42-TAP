#ifndef ITEM_HPP
# define ITEM_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "definition.hpp"
# include "instance.hpp"
# include "respawnable.hpp"

//##############################################################################
//#                                  ITEM DEF                                  #
//##############################################################################

class ItemDef : public Definition {
	private:
		//######################### VARIABLES ##########################
		bool _obtainable;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Builds the definition of an item.
		 *
		 * @param id          id of the definition
		 * @param name        name shown to the players
		 * @param description description shown to the players
		 * @param obtainable  true if a player can take the item
		 */
		ItemDef(const std::string& id, const std::string& name, const std::string& description, bool obtainable);

		//########################## QUERIES ###########################
		/** Tells whether a player can take the item (a fountain cannot be taken). */
		bool isObtainable() const;
};

//##############################################################################
//#                                    ITEM                                    #
//##############################################################################

class Item : public Instance<ItemDef>, public Respawnable {
	public:
		//######################## CONSTRUCTOR #########################
		/** Creates an item from its definition. */
		Item(const ItemDef& def);
};

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

typedef Database<ItemDef, Item> ItemDb;

#endif
