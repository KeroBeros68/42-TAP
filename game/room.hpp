#ifndef ROOM_HPP
# define ROOM_HPP

# include <string>
# include <unordered_map>
# include <vector>

# include "npc.hpp"

struct Spawn {
	std::string	npc;
	int			count = 1;
	int			respawn_seconds = 0;
};

class Room {
	private:
		std::string _id;
		std::string _name;
		std::string _description;

		std::unordered_map<std::string, std::string> _exits;
		std::vector<std::string> _items;
		std::vector<Spawn> _spawns;
		std::unordered_map<std::string, Npc> _alive_npcs;
		std::vector<long long> _current_player;

	public:
		Room(const std::string& id, const std::string& name, const std::string& description,
			const std::unordered_map<std::string, std::string>& exits, const std::vector<std::string>& items,
			const std::vector<Spawn>& spawns
		);

		const std::string& id() const;
		const std::string& name() const;
		const std::string& description() const;
		const std::unordered_map<std::string, std::string>& exits() const;

		const std::vector<std::string>& items() const;
		const std::vector<Spawn>& spawns() const;
		std::vector<std::string> npcs() const;
		const std::unordered_map<std::string, Npc>& aliveNpcs() const;
		void addNpc(const Npc& npc);
		// The NPC designated by a player: its instance id, the id of its definition (with or without "npc."),
		// its name or a word of its name, in any case. nullptr if there is none.
		const Npc* findNpc(const std::string& query) const;

		const std::vector<long long>& currentPlayer() const;
		void addPlayer(long long id);
		void removePlayer(long long id);
};

std::string oppositeDirection(const std::string& direction);

using RoomMap = std::unordered_map<std::string, Room>;

#endif