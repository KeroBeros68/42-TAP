#ifndef NPC_HPP
# define NPC_HPP

# include <string>
# include <unordered_map>
# include <vector>

# include "inventory.hpp"
# include "stats.hpp"

# define NPC_ROLE_ENEMY "enemy"

class NpcDef {
	private:
		std::string _id;
		std::string _name;
		std::string _role;
		std::string _description;
		std::vector<std::string> _dialogues;
		Stats _stats;
		std::vector<std::string> _loots;

	public:
		NpcDef(const std::string& id, const std::string& name, const std::string& role,
			const std::string& description, const std::vector<std::string>& dialogues,
			const Stats& stats, const std::vector<std::string>& loots);

		const std::string& id() const;
		const std::string& name() const;
		const std::string& role() const;
		const std::string& description() const;
		const std::string& dialogue() const;
		const std::vector<std::string>& loots() const;
		const Stats& stats() const;
};

class Npc {
	private:
		static inline int _current_id = 0;
		std::string _id;
		const NpcDef* _definition;
		Stats _stats;
		std::string _home_room;
		int _respawn_seconds = 0;

	public:
		Npc(const NpcDef& def);

		const std::string& id() const;
		const NpcDef& definition() const;
		const std::string& name() const;
		const std::string& talk() const;
		const Stats& stats() const;
		Stats& stats();
		std::vector<std::string> dropLoot(Inventory& floor, const ItemDb& items) const;

		void setOrigin(const std::string& room, int respawn_seconds);
		const std::string& homeRoom() const;
		int respawnSeconds() const;
};

class NpcDb {
	private:
		std::unordered_map<std::string, NpcDef> _npcs;

	public:
		bool addNpc(const NpcDef& npc);
		const NpcDef& npc(const std::string& id) const;
		const std::unordered_map<std::string, NpcDef>& npcs() const;
		Npc instantiate(const std::string& id) const;
};

#endif
