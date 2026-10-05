#include "npc.hpp"
#include "../shared/random.hpp"

NpcDef::NpcDef(const std::string& id, const std::string& name, const std::string& role,
	const std::string& description, const std::vector<std::string>& dialogues,
	const Stats& stats, const std::vector<std::string>& loots)
	: _id(id), _name(name), _role(role), _description(description), _dialogues(dialogues), _stats(stats), _loots(loots) {}

const std::string& NpcDef::id() const {
	return _id;
}

const std::string& NpcDef::name() const {
	return _name;
}

const std::string& NpcDef::description() const {
	return _description;
}

const std::string& NpcDef::role() const {
	return _role;
}

const std::string& NpcDef::dialogue() const {
	static const std::string empty;

	if (_dialogues.empty())
		return empty;

	return _dialogues[randomIndex(_dialogues.size())];
}

const Stats& NpcDef::stats() const {
	return _stats;
}

bool NpcDb::addNpc(const NpcDef& npc) {
	return _npcs.emplace(npc.id(), npc).second;
}

const NpcDef& NpcDb::npc(const std::string& id) const {
	return _npcs.at(id);
}

const std::unordered_map<std::string, NpcDef>& NpcDb::npcs() const {
	return _npcs;
}
