#ifndef RESPAWNABLE_HPP
# define RESPAWNABLE_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                RESPAWNABLE                                 #
//##############################################################################

class Respawnable {
	private:
		//######################### VARIABLES ##########################
		std::string _home_room;
		int _respawn_seconds = 0;

	public:
		//########################### ORIGIN ###########################
		/**
		 * Records the spawn that created the object.
		 *
		 * @param room            id of the room of the spawn
		 * @param respawn_seconds delay before it comes back, 0 if it does not
		 */
		void setOrigin(const std::string& room, int respawn_seconds) {
			_home_room = room;
			_respawn_seconds = respawn_seconds;
		}

		/** Forgets the spawn: the object does not come back any more. */
		void clearOrigin() {
			_home_room.clear();
			_respawn_seconds = 0;
		}

		//########################## GETTERS ###########################
		/** Returns the room of the spawn (empty if there is none). */
		const std::string& homeRoom() const {return _home_room;}

		/** Returns the delay before the object comes back, 0 if it does not. */
		int respawnSeconds() const {return _respawn_seconds;}
};

#endif
