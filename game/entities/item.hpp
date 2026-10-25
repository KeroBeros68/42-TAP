#ifndef ITEM_HPP
# define ITEM_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <optional>
# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "definition.hpp"
# include "instance.hpp"
# include "respawnable.hpp"

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class ItemEffectType { HEALING };

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct ItemEffect {
	ItemEffectType	type = ItemEffectType::HEALING;
	int				amount = 0;
};

//##############################################################################
//#                                  ITEM DEF                                  #
//##############################################################################

class ItemDef : public Definition {
	private:
		//######################### VARIABLES ##########################
		bool _obtainable;
		std::optional<ItemEffect> _effect;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Builds the definition of an item.
		 *
		 * @param id          id of the definition
		 * @param name        name shown to the players
		 * @param description description shown to the players
		 * @param obtainable  true if a player can take the item
		 * @param effect      what the item does when it is used (none by default)
		 */
		ItemDef(const std::string& id, const std::string& name, const std::string& description, bool obtainable,
			const std::optional<ItemEffect>& effect = std::nullopt);

		//########################## QUERIES ###########################
		/** Tells whether a player can take the item (a fountain cannot be taken). */
		bool isObtainable() const;

		/** Returns what the item does when it is used (nothing if it has no effect). */
		const std::optional<ItemEffect>& effect() const;
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
