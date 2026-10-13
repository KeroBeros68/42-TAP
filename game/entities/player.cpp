#include "player.hpp"
#include "../../shared/defines.hpp"

Player::Player(long long id, const std::string& name, const std::string& map)
	: _id(id), _name(name), _current_map(map), _inventory(PLAYER_INVENTORY_CAPACITY) {}

long long Player::id() const {
	return _id;
}

const std::string& Player::name() const {
	return _name;
}

const std::string& Player::currentMap() const {
	return _current_map;
}

void Player::setCurrentMap(const std::string& map) {
	_current_map = map;
}

Inventory& Player::inventory() {
	return _inventory;
}

const Inventory& Player::inventory() const {
	return _inventory;
}
