#ifndef RESPAWNABLE_HPP
# define RESPAWNABLE_HPP

# include <string>

class Respawnable {
	private:
		std::string _home_room;
		int _respawn_seconds = 0;

	public:
		void setOrigin(const std::string& room, int respawn_seconds) {
			_home_room = room;
			_respawn_seconds = respawn_seconds;
		}

		void clearOrigin() {
			_home_room.clear();
			_respawn_seconds = 0;
		}
		const std::string& homeRoom() const {return _home_room;}
		int respawnSeconds() const { return _respawn_seconds;}
};

#endif