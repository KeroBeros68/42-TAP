#include "npc.hpp"
#include "../../shared/defines.hpp"
#include "../../shared/random.hpp"

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

Npc NpcDb::instantiate(const std::string& id) const {
	return Npc(npc(id));
}

Npc::Npc(const NpcDef& def)
	: _id(def.id() + INSTANCE_SEPARATOR + std::to_string(++_current_id)), _definition(&def), _stats(def.stats()) {}

const std::string& Npc::id() const {
	return _id;
}

const NpcDef& Npc::definition() const {
	return *_definition;
}

const std::string& Npc::name() const {
	return _definition->name();
}

const std::string& Npc::talk() const {
	return _definition->dialogue();
}
