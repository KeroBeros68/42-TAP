#ifndef SERIALIZER_HPP
# define SERIALIZER_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <optional>
# include <string>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../game/game.hpp"

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Writes the response of LOOK.
 *
 * @param look the room and the players in it
 * @return     a JSON object with the room, its exits, the players, the items and the npcs
 */
std::string serializeLook(const LookData& look);

/**
 * Writes the response of STATUS.
 *
 * @param stats  the stats of the player
 * @param combat his fight, if any
 * @return       a JSON object with the hit points and the status, and the fight when there is one
 */
std::string serializeStatus(const Stats& stats, const std::optional<CombatView>& combat);

/**
 * Writes the response of INVENTORY.
 *
 * @param ids the instance ids of the items of the player
 * @return    a JSON array of the ids
 */
std::string serializeInventory(const std::vector<std::string>& ids);

/**
 * Writes the response of QUEST.
 *
 * @param reply the quest concerned, and what happened (STARTED, IN_PROGRESS or COMPLETED)
 * @return      a JSON object with the quest, its status, its description, the progress and the reward, and what the npc says at the end when it is completed
 */
std::string serializeQuest(const QuestReply& reply);

/**
 * Writes the response of QUESTS.
 *
 * @param quests the quests of the player
 * @return       a JSON array with, for each quest, its id, name, status and progress
 */
std::string serializeQuests(const std::vector<QuestView>& quests);

/**
 * Writes the response of ATTACK, DEFEND and FLEE.
 *
 * @param report what happened
 * @return       a JSON object with the hit points, the damage, the round and the turns played
 */
std::string serializeAttack(const CombatReport& report);

#endif
