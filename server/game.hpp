#ifndef GAME_HPP
# define GAME_HPP

# include <string>
# include <unordered_map>
# include <vector>

# include "user.hpp"
# include "../game/world.hpp"


class Game {
	private:
		struct Player {
			std::string	name;
			std::string	current_map;
			// later: hp, inventory, group, quests...
		};
		std::unordered_map<long long, Player> _players;

		World _world;

	public:
		Game();

		void addPlayer(const User& user);
		void removePlayer(long long id);

		const World& world() const;
		std::string look(long long player_id) const;
		const std::string& playerMap(long long id) const;

		bool move(long long player_id, const std::string& direction);

		bool hasPlayer(long long id) const;
		bool isNameTaken(const std::string& name) const;
		size_t playerCount() const;
		std::vector<long long> playerIds() const;
};

#endif
