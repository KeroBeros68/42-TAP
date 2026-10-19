#ifndef SPAWN_HPP
# define SPAWN_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "room.hpp"

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Creates the object of a definition the way a spawn does, and puts it in its room.
 * The object remembers the spawn (its room and when it comes back).
 *
 * @param database        npcs or items, to create the object
 * @param def_id          id of the definition
 * @param room_id         id of the room of the spawn
 * @param respawn_seconds delay before it comes back, 0 if it does not
 * @param room            the room where the object is put
 * @param place           puts the object in the room: place(room, object)
 * @return                the object that was created
 */
template <typename Db, typename Place>
auto spawnInstance(const Db& database, const std::string& def_id, const std::string& room_id, int respawn_seconds, Room& room, Place place) {
	auto instance = database.instantiate(def_id);
	instance.setOrigin(room_id, respawn_seconds);
	place(room, instance);
	return instance;
}

#endif
