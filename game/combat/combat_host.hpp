#ifndef COMBAT_HOST_HPP
# define COMBAT_HOST_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../events/event_bus.hpp"
# include "../world/presence.hpp"

//##############################################################################
//#                                COMBAT HOST                                 #
//##############################################################################

class CombatHost {
	public:
		//######################### DESTRUCTOR #########################
		/** Destroys the host. */
		virtual ~CombatHost() {}

		//########################### LOOKUP ###########################
		/**
		 * Finds a player and the room where he is.
		 *
		 * @param player_id id of the player
		 * @param action    what is asked, for the log if he is unknown
		 * @return          the player and his room (both null if one is unknown)
		 */
		virtual Presence locate(long long player_id, const char* action) = 0;

		/**
		 * Finds a player.
		 *
		 * @param player_id id of the player
		 * @return          the player, nullptr if he is unknown
		 */
		virtual Player* findPlayer(long long player_id) = 0;

		/**
		 * Finds a room.
		 *
		 * @param room_id id of the room
		 * @return        the room, nullptr if it is unknown
		 */
		virtual Room* findRoom(const std::string& room_id) = 0;

		//########################### EVENTS ###########################
		/** Returns the queue where the events are put. */
		virtual EventBus& events() = 0;

		//########################## ACTIONS ###########################
		/**
		 * Moves a player to another room, with no rule checked.
		 *
		 * @param player_id id of the player
		 * @param room_id   id of the room
		 * @return          false if the player or the room is unknown
		 */
		virtual bool teleport(long long player_id, const std::string& room_id) = 0;

		/**
		 * Takes care of an enemy that was killed: its loot, its respawn, and its removal from the room.
		 *
		 * @param room   the room of the enemy
		 * @param npc    the enemy, which is not valid afterwards
		 * @param killer       name of the player who killed it, for the log
		 * @param participants ids of the players who were in the fight
		 */
		virtual void enemyDied(Room& room, Npc* npc, const std::string& killer, const std::vector<long long>& participants) = 0;

		//########################## RESPAWN ###########################
		/** Returns the id of the room where a dead player comes back. */
		virtual const std::string& respawnRoom() const = 0;

		/** Returns the hit points a dead player has when he comes back. */
		virtual int respawnLife() const = 0;
};

#endif
