//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "player.hpp"
# include "../../shared/defines.hpp"

//##############################################################################
//#                                   PLAYER                                   #
//##############################################################################

//######################## CONSTRUCTOR #########################

Player::Player(long long id, const std::string& name, const std::string& map)
	: _id(id), _name(name), _current_map(map), _inventory(PLAYER_INVENTORY_CAPACITY), _stats(PLAYER_MAX_HP, PLAYER_MAX_HP, PLAYER_ATTACK, PLAYER_DEFENSE, PLAYER_HIT, PLAYER_FLEE) {}

//########################## IDENTITY ##########################

long long Player::id() const {
	return _id;
}

const std::string& Player::name() const {
	return _name;
}

//########################## LOCATION ##########################

const std::string& Player::currentMap() const {
	return _current_map;
}

void Player::setCurrentMap(const std::string& map) {
	_current_map = map;
}

//########################### STATE ############################

Inventory& Player::inventory() {
	return _inventory;
}

const Inventory& Player::inventory() const {
	return _inventory;
}

const Stats& Player::stats() const {
	return _stats;
}

Stats& Player::stats() {
	return _stats;
}
