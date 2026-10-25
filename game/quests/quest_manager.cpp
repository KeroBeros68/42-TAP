//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "quest_manager.hpp"

//##############################################################################
//#                               QUEST MANAGER                                #
//##############################################################################

//########################### LOOKUP ###########################

std::vector<const QuestProgress*> QuestManager::list(long long player_id) const {
	std::vector<const QuestProgress*> quests;
	auto player = _progress.find(player_id);
	if (player == _progress.end())
		return quests;
	for (const auto& [id, progress] : player->second)
		quests.push_back(&progress);
	return quests;
}

QuestProgress* QuestManager::activeFor(long long player_id, const std::string& npc_def) {
	auto player = _progress.find(player_id);
	if (player == _progress.end())
		return nullptr;
	for (auto& [id, progress] : player->second) {
		if (progress.state() == QuestState::ACTIVE && progress.definition().turnIn() == npc_def)
			return &progress;
	}
	return nullptr;
}

const QuestDef* QuestManager::offeredBy(const QuestDb& quests, long long player_id, const std::string& npc_def) const {
	auto player = _progress.find(player_id);
	const QuestDef* offered = nullptr;
	for (const auto& [id, def] : quests.all()) {
		if (def.giver() != npc_def)
			continue;
		if (player != _progress.end() && player->second.count(id))
			continue;
		if (!offered || id < offered->id())
			offered = &def;
	}
	return offered;
}

//########################## CHANGES ###########################

QuestProgress& QuestManager::start(long long player_id, const QuestDef& def) {
	return _progress[player_id].emplace(def.id(), QuestProgress(def)).first->second;
}

std::vector<const QuestProgress*> QuestManager::recordKill(long long player_id, const std::string& enemy_def) {
	std::vector<const QuestProgress*> counted;
	auto player = _progress.find(player_id);
	if (player == _progress.end())
		return counted;
	for (auto& [id, progress] : player->second) {
		const QuestDef& def = progress.definition();
		if (progress.state() == QuestState::ACTIVE && def.type() == QuestType::DEFEAT && def.target() == enemy_def) {
			progress.addKill();
			counted.push_back(&progress);
		}
	}
	return counted;
}

void QuestManager::remove(long long player_id) {
	_progress.erase(player_id);
}
