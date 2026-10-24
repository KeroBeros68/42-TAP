//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <algorithm>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "group_manager.hpp"

//##############################################################################
//#                               GROUP MANAGER                                #
//##############################################################################

//######################### INVITATIONS ########################

void GroupManager::cancelInvitationsFrom(long long inviter) {
	for (auto it = _invitations.begin(); it != _invitations.end();) {
		if (it->second.inviter == inviter)
			it = _invitations.erase(it);
		else
			++it;
	}
}

//########################### LOOKUP ###########################

std::optional<std::string> GroupManager::groupOf(long long player_id) const {
	auto it = _group_of.find(player_id);
	if (it == _group_of.end())
		return std::nullopt;
	return it->second;
}

std::vector<long long> GroupManager::membersOf(long long player_id) const {
	auto it = _group_of.find(player_id);
	if (it == _group_of.end())
		return {};
	return _groups.at(it->second).members;
}

//########################## COMMANDS ##########################

GroupResult GroupManager::create(long long player_id) {
	if (_group_of.count(player_id))
		return {GroupStatus::ALREADY_IN_GROUP, "", {}};

	const std::string id = GROUP_ID_PREFIX + std::to_string(_next_id++);
	_groups[id] = {id, {player_id}};
	_group_of[player_id] = id;
	_invitations.erase(player_id);
	return {GroupStatus::DONE, id, {}};
}

GroupResult GroupManager::invite(long long inviter_id, long long invited_id) {
	auto group_id = groupOf(inviter_id);
	if (!group_id)
		return {GroupStatus::NOT_IN_GROUP, "", {}};

	const Group& group = _groups.at(*group_id);
	if (group.members.front() != inviter_id)
		return {GroupStatus::NOT_LEADER, *group_id, {}};
	if (inviter_id == invited_id)
		return {GroupStatus::SELF, *group_id, {}};
	if (_group_of.count(invited_id))
		return {GroupStatus::ALREADY_IN_GROUP, *group_id, {}};
	if (group.members.size() >= MAX_GROUP_SIZE)
		return {GroupStatus::GROUP_FULL, *group_id, {}};

	_invitations[invited_id] = {*group_id, inviter_id};
	return {GroupStatus::DONE, *group_id, {}};
}

GroupResult GroupManager::join(long long player_id, long long member_id) {
	if (_group_of.count(player_id))
		return {GroupStatus::ALREADY_IN_GROUP, "", {}};

	auto group_id = groupOf(member_id);
	auto invitation = _invitations.find(player_id);
	if (!group_id || invitation == _invitations.end() || invitation->second.group_id != *group_id)
		return {GroupStatus::NO_INVITATION, "", {}};

	Group& group = _groups.at(*group_id);
	if (group.members.size() >= MAX_GROUP_SIZE)
		return {GroupStatus::GROUP_FULL, *group_id, {}};

	_invitations.erase(invitation);
	GroupResult result{GroupStatus::DONE, *group_id, group.members};
	group.members.push_back(player_id);
	_group_of[player_id] = *group_id;
	return result;
}

GroupResult GroupManager::leave(long long player_id) {
	auto group_id = groupOf(player_id);
	if (!group_id)
		return {GroupStatus::NOT_IN_GROUP, "", {}};

	Group& group = _groups.at(*group_id);
	const bool was_leader = group.members.front() == player_id;
	group.members.erase(std::find(group.members.begin(), group.members.end(), player_id));
	_group_of.erase(player_id);
	if (was_leader)
		cancelInvitationsFrom(player_id);

	GroupResult result{GroupStatus::DONE, *group_id, group.members};
	if (group.members.empty()) {
		for (auto it = _invitations.begin(); it != _invitations.end();) {
			if (it->second.group_id == *group_id)
				it = _invitations.erase(it);
			else
				++it;
		}
		_groups.erase(*group_id);
	}
	return result;
}

GroupResult GroupManager::remove(long long player_id) {
	_invitations.erase(player_id);
	return leave(player_id);
}
