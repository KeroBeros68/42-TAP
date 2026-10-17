#ifndef NPC_HPP
# define NPC_HPP

# include <string>
# include <unordered_map>
# include <vector>

# include "definition.hpp"
# include "respawnable.hpp"
# include "instance.hpp"
# include "inventory.hpp"
# include "stats.hpp"

# define NPC_ROLE_ENEMY "enemy"

class NpcDef : public Definition  {
	private:
		std::string _role;
		std::vector<std::string> _dialogues;
		Stats _stats;
		std::vector<std::string> _loots;

	public:
		NpcDef(const std::string& id, const std::string& name, const std::string& role,
			const std::string& description, const std::vector<std::string>& dialogues,
			const Stats& stats, const std::vector<std::string>& loots);

		const std::string& role() const;
		const std::string& dialogue() const;
		const std::vector<std::string>& loots() const;
		const Stats& stats() const;
};

class Npc : public Instance<NpcDef>, public Respawnable {
	private:
		Stats _stats;

	public:
		Npc(const NpcDef& def);

		const std::string& talk() const;
		const Stats& stats() const;
		Stats& stats();
		std::vector<std::string> dropLoot(Inventory& floor, const ItemDb& items) const;
};

typedef Database<NpcDef, Npc> NpcDb;

#endif
