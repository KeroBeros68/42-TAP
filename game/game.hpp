#ifndef GAME_HPP
# define GAME_HPP

# include <chrono>
# include <map>
# include <optional>
# include <string>
# include <unordered_map>
# include <vector>

# include "entities/player.hpp"
# include "combat/combat.hpp"
# include "world/world.hpp"

# define LOOT_SEPARATOR	","

struct LookData {
	const Room* room;
	std::vector<std::string> players;
};

# define LOG_HERE(here) \
	{"client", (here).player->id()}, {"player", (here).player->name()}, {"room", (here).room->id()}

class Game {
	private:
		std::unordered_map<long long, Player> _players;

		World _world;

		enum class RespawnKind { Npc, Item };
		struct Respawn {
			RespawnKind kind;
			std::string def_id;
			std::string room_id;
			int seconds;

			template <typename Spawned>
			static Respawn of(RespawnKind kind, const Spawned& spawned) {
				return {kind, spawned.definition().id(), spawned.homeRoom(), spawned.respawnSeconds()};
			}
		};
		std::multimap<std::chrono::steady_clock::time_point, Respawn> _respawns;

		bool schedule(const Respawn& respawn);

		template <typename P, typename R>
		struct BasicPresence {
			P* player = nullptr;
			R* room = nullptr;
			explicit operator bool() const { return player && room; }
		};
		typedef BasicPresence<Player, Room>				Presence;
		typedef BasicPresence<const Player, const Room>	ConstPresence;

		ConstPresence locate(long long player_id, const char* action) const;
		Presence locate(long long player_id, const char* action);

		bool teleport(long long player_id, const std::string& target);

	public:
		Game();

		void addPlayer(long long id, const std::string& name);
		void removePlayer(long long id);

		const World& world() const;
		std::optional<LookData> look(long long player_id) const;
		const std::string& playerMap(long long id) const;

		bool move(long long player_id, const std::string& direction);

		TransferResult take(long long player_id, const std::string& target);
		TransferResult drop(long long player_id, const std::string& target);

		std::optional<std::string> talk(long long player_id, const std::string& target) const;

		CombatResult attack(long long player_id, const std::string& target);

		const Stats* status(long long player_id) const;

		void tick(std::chrono::steady_clock::time_point now);

		bool hasPlayer(long long id) const;
		bool isNameTaken(const std::string& name) const;
		size_t playerCount() const;
		std::vector<long long> playerIds() const;
};

#endif
