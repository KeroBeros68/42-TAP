#ifndef ROOM_HPP
# define ROOM_HPP

# include <string>
# include <unordered_map>
# include <vector>
# include <optional>

# include "../entities/npc.hpp"
# include "../entities/inventory.hpp"
# include "../utils/lookup.hpp"

struct Spawn {
	std::string	id;
	int			count = 1;
	int			respawn_seconds = 0;
};

class Room {
	private:
		std::string _id;
		std::string _name;
		std::string _description;

		std::unordered_map<std::string, std::string> _exits;
		std::vector<Spawn> _item_spawns;
		std::vector<Spawn> _npc_spawns;
		std::unordered_map<std::string, Npc> _alive_npcs;
		Inventory _inventory;
		std::vector<long long> _current_player;

	public:
		Room(const std::string& id, const std::string& name, const std::string& description,
			const std::unordered_map<std::string, std::string>& exits,
			const std::vector<Spawn>& item_spawns, const std::vector<Spawn>& npc_spawns
		);

		const std::string& id() const;
		const std::string& name() const;
		const std::string& description() const;
		const std::unordered_map<std::string, std::string>& exits() const;

		const std::vector<Spawn>& itemSpawns() const;
		const std::vector<Spawn>& npcSpawns() const;
		std::vector<std::string> npcs() const;
		std::vector<std::string> items() const;
		const std::unordered_map<std::string, Npc>& aliveNpcs() const;
		void addNpc(const Npc& npc);
		const Npc* findNpc(const std::string& query) const;
		Npc* findNpc(const std::string& query);

		Inventory& inventory();
		const Inventory& inventory() const;

		const std::vector<long long>& currentPlayer() const;
		void addPlayer(long long id);
		void removePlayer(long long id);

		void removeNpc(Npc* npc);
};

std::string oppositeDirection(const std::string& direction);

using RoomMap = std::unordered_map<std::string, Room>;

#endif