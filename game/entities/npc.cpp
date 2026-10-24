//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "npc.hpp"
# include "../../shared/defines.hpp"
# include "../../shared/system/random.hpp"

//##############################################################################
//#                                  NPC DEF                                   #
//##############################################################################

//######################## CONSTRUCTOR #########################

NpcDef::NpcDef(const std::string& id, const std::string& name, const std::string& role,
	const std::string& description, const std::vector<std::string>& dialogues,
	const Stats& stats, const std::vector<std::string>& loots)
	: Definition(id, name, description), _role(role), _dialogues(dialogues), _stats(stats), _loots(loots) {}

//########################## GETTERS ###########################

const std::string& NpcDef::role() const {
	return _role;
}

const std::string& NpcDef::dialogue() const {
	static const std::string empty;

	if (_dialogues.empty())
		return empty;

	return _dialogues[randomIndex(_dialogues.size())];
}

const std::vector<std::string>& NpcDef::loots() const {
	return _loots;
}

const Stats& NpcDef::stats() const {
	return _stats;
}

//##############################################################################
//#                                    NPC                                     #
//##############################################################################

//######################## CONSTRUCTOR #########################

Npc::Npc(const NpcDef& def)
	: Instance(def), _stats(def.stats()) {}

//######################### FUNCTIONS ##########################

const std::string& Npc::talk() const {
	return definition().dialogue();
}

const Stats& Npc::stats() const {
	return _stats;
}

Stats& Npc::stats() {
	return _stats;
}

std::vector<std::string> Npc::dropLoot(Inventory& floor, const ItemDb& items) const {
	std::vector<std::string> dropped;
	for (const std::string& loot : definition().loots()) {
		Item item = items.instantiate(loot);
		dropped.push_back(item.id());
		floor.add(item);
	}
	return dropped;
}
