#ifndef NPCDB_HPP
# define NPCDB_HPP

# include <string>
# include <unordered_map>

# include "npcDef.hpp"

class NpcDb {
	private:
		std::unordered_map<std::string, NpcDef> _npcs;

	public:
		bool addNpc(const NpcDef& npc);
		const NpcDef& npc(const std::string& id) const;
		const std::unordered_map<std::string, NpcDef>& npcs() const;
};

#endif
