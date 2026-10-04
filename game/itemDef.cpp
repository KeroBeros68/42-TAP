#include "itemDef.hpp"

ItemDef::ItemDef(const std::string& id, const std::string& name, const std::string& description, bool obtainable) 
	: _id(id), _name(name), _description(description), _obtainable(obtainable) {}

const std::string& ItemDef::id() const {
	return _id;
}

const std::string& ItemDef::name() const {
	return _name;
}

const std::string& ItemDef::description() const {
	return _description;
}

bool ItemDef::isObtainable() const {
	return _obtainable;
}