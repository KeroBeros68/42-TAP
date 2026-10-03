#ifndef GAME_HPP
# define GAME_HPP

# include <string>
# include <unordered_map>
# include <vector>

# include "user.hpp"

// State of the world, independent from the network (no socket, no poll).
// The commands receive it when they are registered (see commands.cpp).
class Game {
	private:
		struct Player {
			std::string	name;
			// later: current room, hp, inventory, group, quests...
		};
		std::unordered_map<long long, Player> _players;

	public:
		Game();

		// A player exists in the game once its user is authenticated
		void addPlayer(const User& user);
		void removePlayer(long long id);

		bool hasPlayer(long long id) const;
		bool isNameTaken(const std::string& name) const;
		size_t playerCount() const;
		std::vector<long long> playerIds() const;
};

#endif
