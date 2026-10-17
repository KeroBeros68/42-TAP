#ifndef SPAWN_HPP
# define SPAWN_HPP

# include <string>

# include "room.hpp"

template <typename Db, typename Place>
auto spawnInstance(const Db& database, const std::string& def_id, const std::string& room_id, int respawn_seconds, Room& room, Place place) {
	auto instance = database.instantiate(def_id);
	instance.setOrigin(room_id, respawn_seconds);
	place(room, instance);
	return instance;
}

#endif
