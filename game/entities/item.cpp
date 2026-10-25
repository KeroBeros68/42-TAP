//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "item.hpp"
# include "../../shared/defines.hpp"

//##############################################################################
//#                                  ITEM DEF                                  #
//##############################################################################

//######################## CONSTRUCTOR #########################

ItemDef::ItemDef(const std::string& id, const std::string& name, const std::string& description, bool obtainable,
	const std::optional<ItemEffect>& effect)
	: Definition(id, name, description), _obtainable(obtainable), _effect(effect) {}

//########################## QUERIES ###########################

bool ItemDef::isObtainable() const {
	return _obtainable;
}

const std::optional<ItemEffect>& ItemDef::effect() const {
	return _effect;
}

//##############################################################################
//#                                    ITEM                                    #
//##############################################################################

//######################## CONSTRUCTOR #########################

Item::Item(const ItemDef& def)
	: Instance(def) {}
