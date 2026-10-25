#ifndef QUEST_MANAGER_HPP
# define QUEST_MANAGER_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <map>
# include <string>
# include <unordered_map>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "quest.hpp"

//##############################################################################
//#                               QUEST MANAGER                                #
//##############################################################################

class QuestManager {
	private:
		//######################### VARIABLES ##########################
		std::unordered_map<long long, std::map<std::string, QuestProgress>> _progress;

	public:
		//########################### LOOKUP ###########################
		/**
		 * Lists the quests of a player, given or finished.
		 *
		 * @param player_id id of the player
		 * @return          their progress, in the order of the quest ids
		 */
		std::vector<const QuestProgress*> list(long long player_id) const;

		/**
		 * Finds the quest in progress that a player has to give back to an npc.
		 *
		 * @param player_id id of the player
		 * @param npc_def   id of the definition of the npc
		 * @return          the progress (the first by quest id if several), nullptr if there is none
		 */
		QuestProgress* activeFor(long long player_id, const std::string& npc_def);

		/**
		 * Finds the quest that an npc can give to a player: one he never had.
		 *
		 * @param quests    all the quests
		 * @param player_id id of the player
		 * @param npc_def   id of the definition of the npc
		 * @return          the definition (the first by id if several), nullptr if there is none
		 */
		const QuestDef* offeredBy(const QuestDb& quests, long long player_id, const std::string& npc_def) const;

		//########################## CHANGES ###########################
		/**
		 * Gives a quest to a player.
		 *
		 * @param player_id id of the player
		 * @param def       the quest, which must outlive the progress
		 * @return          the progress
		 */
		QuestProgress& start(long long player_id, const QuestDef& def);

		/**
		 * Counts a defeated enemy for the players of the fight that have a quest to defeat it.
		 *
		 * @param player_id id of a player of the fight
		 * @param enemy_def id of the definition of the enemy
		 * @return          the quests of this player that counted it
		 */
		std::vector<const QuestProgress*> recordKill(long long player_id, const std::string& enemy_def);

		/**
		 * Forgets the quests of a player who left the game.
		 *
		 * @param player_id id of the player
		 */
		void remove(long long player_id);
};

#endif
