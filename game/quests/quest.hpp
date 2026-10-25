#ifndef QUEST_HPP
# define QUEST_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../entities/definition.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Quest types --------------------------------------------------------------
# define QUEST_TYPE_DELIVER	"deliver"	// bring an item to another npc
# define QUEST_TYPE_DEFEAT	"defeat"	// kill enemies
# define QUEST_TYPE_FETCH	"fetch"		// find an item and bring it back

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class QuestType { DELIVER, DEFEAT, FETCH };

enum class QuestState { ACTIVE, COMPLETED };

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct QuestReward {
	std::vector<std::string>	items;
	int							heal = 0;
};

//##############################################################################
//#                                  QUEST DEF                                 #
//##############################################################################

class QuestDef : public Definition {
	private:
		//######################### VARIABLES ##########################
		QuestType _type;
		std::string _giver;
		std::string _turn_in;
		std::string _target;
		int _count;
		QuestReward _reward;
		std::string _text_start;
		std::string _text_done;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Builds the definition of a quest. The description is the text said when the quest is given.
		 *
		 * @param id         id of the definition, like "quest.cellar_pests"
		 * @param name       name shown to the players
		 * @param type       what has to be done
		 * @param giver      id of the npc definition who gives the quest
		 * @param turn_in    id of the npc definition who receives the result
		 * @param target     id of the item to bring, or of the enemy to defeat
		 * @param count      how many items or enemies
		 * @param reward     what the player gets at the end
		 * @param text_start what the giver says when he gives the quest
		 * @param text_done  what the npc says at the end
		 */
		QuestDef(const std::string& id, const std::string& name, QuestType type,
			const std::string& giver, const std::string& turn_in, const std::string& target,
			int count, const QuestReward& reward, const std::string& text_start, const std::string& text_done);

		//########################## GETTERS ###########################
		/** Returns what has to be done. */
		QuestType type() const;

		/** Returns the id of the npc definition who gives the quest. */
		const std::string& giver() const;

		/** Returns the id of the npc definition who receives the result. */
		const std::string& turnIn() const;

		/** Returns the id of the item to bring, or of the enemy to defeat. */
		const std::string& target() const;

		/** Returns how many items or enemies. */
		int count() const;

		/** Returns what the player gets at the end. */
		const QuestReward& reward() const;

		/** Returns what the giver says when he gives the quest. */
		const std::string& textStart() const;

		/** Returns what the npc says at the end. */
		const std::string& textDone() const;
};

//##############################################################################
//#                               QUEST PROGRESS                               #
//##############################################################################

class QuestProgress {
	private:
		//######################### VARIABLES ##########################
		const QuestDef* _definition;
		QuestState _state = QuestState::ACTIVE;
		int _kills = 0;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Starts a quest for a player.
		 *
		 * @param def the definition, which must outlive the progress
		 */
		explicit QuestProgress(const QuestDef& def);

		//########################## GETTERS ###########################
		/** Returns the definition of the quest. */
		const QuestDef& definition() const;

		/** Returns whether the quest is in progress or finished. */
		QuestState state() const;

		/** Returns how many enemies were defeated since the quest was given. */
		int kills() const;

		//########################## FUNCTIONS #########################
		/** Counts an enemy defeated, up to the number asked. */
		void addKill();

		/** Marks the quest as finished. */
		void complete();
};

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

typedef Database<QuestDef, QuestProgress> QuestDb;

#endif
