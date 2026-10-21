#ifndef GROUP_MANAGER_HPP
# define GROUP_MANAGER_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <optional>
# include <string>
# include <unordered_map>
# include <vector>

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Groups -------------------------------------------------------------------
# define GROUP_ID_PREFIX	"group."	// beginning of the id of a group, followed by a counter
# define MAX_GROUP_SIZE		4			// maximum number of players in a group

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class GroupStatus { DONE, NOT_IN_GROUP, ALREADY_IN_GROUP, NOT_LEADER, GROUP_FULL, NO_INVITATION, SELF };

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct GroupResult {
	GroupStatus status = GroupStatus::DONE;
	std::string group_id{};
	std::vector<long long> others{};
};

//##############################################################################
//#                               GROUP MANAGER                                #
//##############################################################################

class GroupManager {
	private:
		//######################### STRUCTURES #########################
		struct Group {
			std::string id;
			std::vector<long long> members;
		};

		struct Invitation {
			std::string group_id;
			long long inviter;
		};

		//######################### VARIABLES ##########################
		std::unordered_map<std::string, Group> _groups;
		std::unordered_map<long long, std::string> _group_of;
		std::unordered_map<long long, Invitation> _invitations;
		unsigned long long _next_id = 1;

		//######################### INVITATIONS ########################
		/**
		 * Cancels the invitations that a player sent.
		 *
		 * @param inviter id of the player
		 */
		void cancelInvitationsFrom(long long inviter);

	public:
		//########################### LOOKUP ###########################
		/**
		 * Returns the id of the group of a player.
		 *
		 * @param player_id id of the player
		 * @return          the id of the group, nothing if he is in none
		 */
		std::optional<std::string> groupOf(long long player_id) const;

		/**
		 * Lists the members of the group of a player (himself included).
		 *
		 * @param player_id id of the player
		 * @return          the ids of the members, the leader first (nothing if he is in no group)
		 */
		std::vector<long long> membersOf(long long player_id) const;

		//########################## COMMANDS ##########################
		/**
		 * Creates a group; the player is its leader.
		 *
		 * @param player_id id of the player
		 * @return          DONE and the id of the group, or ALREADY_IN_GROUP
		 */
		GroupResult create(long long player_id);

		/**
		 * Invites a player in the group of the inviter. A new invitation replaces the previous one of the invited player.
		 *
		 * @param inviter_id id of the inviter, who must be the leader
		 * @param invited_id id of the invited player
		 * @return           DONE and the id of the group; NOT_IN_GROUP, NOT_LEADER, SELF, ALREADY_IN_GROUP (the invited player has a group) or GROUP_FULL
		 */
		GroupResult invite(long long inviter_id, long long invited_id);

		/**
		 * Joins the group of a player, which needs an invitation to it.
		 *
		 * @param player_id id of the player who joins
		 * @param member_id id of a member of the group to join
		 * @return          DONE, the id of the group and the other members; ALREADY_IN_GROUP, NO_INVITATION or GROUP_FULL
		 */
		GroupResult join(long long player_id, long long member_id);

		/**
		 * Leaves the group. The oldest member becomes the leader if the leader leaves, and an empty group is deleted.
		 *
		 * @param player_id id of the player
		 * @return          DONE, the id of the group and the members who stay, or NOT_IN_GROUP
		 */
		GroupResult leave(long long player_id);

		/**
		 * Forgets a player who left the game: he leaves his group and his invitations are cancelled.
		 *
		 * @param player_id id of the player
		 * @return          what leave() returns for him
		 */
		GroupResult remove(long long player_id);
};

#endif
