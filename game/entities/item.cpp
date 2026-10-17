#include "item.hpp"
#include "../../shared/defines.hpp"

ItemDef::ItemDef(const std::string& id, const std::string& name, const std::string& description, bool obtainable) 
	: Definition(id, name, description), _obtainable(obtainable) {}


bool ItemDef::isObtainable() const {
	return _obtainable;
}

Item::Item(const ItemDef& def)
	: Instance(def) {}
