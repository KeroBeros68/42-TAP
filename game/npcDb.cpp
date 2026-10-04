#include "npcDb.hpp"

bool NpcDb::addNpc(const NpcDef& npc) {
	return _npcs.emplace(npc.id(), npc).second;
}

const NpcDef& NpcDb::npc(const std::string& id) const {
	return _npcs.at(id);
}

const std::unordered_map<std::string, NpcDef>& NpcDb::npcs() const {
	return _npcs;
}
