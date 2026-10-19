#ifndef COMBAT_MANAGER_HPP
# define COMBAT_MANAGER_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <chrono>
# include <optional>
# include <string>
# include <unordered_map>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "combat.hpp"
# include "combat_host.hpp"

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class CombatStatus { DONE, NOT_FOUND, NOT_HOSTILE, NOT_IN_COMBAT, ANOTHER_ENEMY, NOT_YOUR_TURN };

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct ReportTurn {
	std::string actor;
	std::string target;
	int round;
	CombatEvent event;
};

struct CombatReport {
	CombatStatus status = CombatStatus::DONE;
	std::vector<ReportTurn> turns{};
	int round = 0;
	int damage = 0;
	int player_hp = 0;
	int enemy_hp = 0;
	Status player_status = Status::HEALTHY;
	bool escaped = false;
};

struct CombatView {
	std::string enemy_id;
	int round;
	std::string turn;
	bool your_turn;
	int seconds_left;
};

//##############################################################################
//#                               COMBAT MANAGER                               #
//##############################################################################

class CombatManager {
	private:
		//########################### TYPES ############################
		struct Fight {
			Encounter encounter;
			std::string room_id;
			std::chrono::steady_clock::time_point deadline;
			long long announced_actor = NO_ACTOR;
			int announced_round = 0;
		};

		//######################### VARIABLES ##########################
		CombatHost& _host;
		std::unordered_map<std::string, Fight> _fights;
		std::unordered_map<long long, std::string> _fighting;

		//########################## FIGHTERS ##########################
		/**
		 * Builds the function that gives the stats of a fighter of the fights against an enemy.
		 *
		 * @param npc the enemy
		 * @return    a function from an id (a player, or ENEMY_ID) to his stats
		 */
		StatsOf statsOf(Npc* npc);

		/**
		 * Finds the name of a fighter.
		 *
		 * @param id       a player id, or ENEMY_ID
		 * @param enemy_id id of the enemy
		 * @return         the name of the player, or the id of the enemy
		 */
		std::string fighterName(long long id, const std::string& enemy_id) const;

		//########################### EVENTS ###########################
		/**
		 * Queues an event of combat for the players of a room but one.
		 *
		 * @param room      the room of the fight
		 * @param except_id id of the player who has his answer (NO_ACTOR if nobody has)
		 * @param words     what follows "EVT ROOM COMBAT"
		 */
		void emitRoomCombat(const Room& room, long long except_id, const std::vector<std::string>& words);

		//########################### TURNS ############################
		/**
		 * Plays an action of a player who is already in a fight.
		 *
		 * @param player_id id of the player
		 * @param action    what he does
		 * @param name      name of the command, for the log
		 * @return          what happened, or why nothing did
		 */
		CombatReport continueFight(long long player_id, CombatAction action, const char* name);

		/**
		 * Plays the turn of a player and the turns that follow it, then tells and applies what happened.
		 *
		 * @param here   the player and his room
		 * @param fight  his fight
		 * @param npc    his enemy
		 * @param action what he does
		 * @return       the report of the turns, or NOT_YOUR_TURN if it was not his turn
		 */
		CombatReport takeTurn(Presence& here, Fight& fight, Npc* npc, CombatAction action);

		/**
		 * Tells and applies what happened: the turns, the players who died or fled, the end of the fight, the next turn.
		 * The fight is not valid any more when it is over.
		 *
		 * @param fight        the fight
		 * @param npc          the enemy
		 * @param room         the room of the fight
		 * @param steps        what happened
		 * @param requester_id the player who has his answer (NO_ACTOR if nobody has)
		 * @param report       receives his answer, nullptr if nobody has one
		 */
		void settle(Fight& fight, Npc* npc, Room& room, const Steps& steps, long long requester_id, CombatReport* report);

		/**
		 * Tells the room who has to play, and starts his delay. Nothing is told again if the turn did not change.
		 *
		 * @param fight the fight
		 * @param room  the room of the fight
		 */
		void announceTurn(Fight& fight, const Room& room);

		//########################## ENDINGS ###########################
		/**
		 * Ends a fight: its players and its enemy are not in combat any more.
		 *
		 * @param enemy_id id of the enemy of the fight
		 */
		void dropFight(const std::string& enemy_id);

		/**
		 * Takes care of a player who died: tells the room, then sends him to the respawn room with his hit points back.
		 *
		 * @param player_id    id of the player
		 * @param enemy_id     id of the enemy that killed him
		 * @param requester_id the player who has his answer (NO_ACTOR if nobody has)
		 */
		void playerDied(long long player_id, const std::string& enemy_id, long long requester_id);

		/**
		 * Takes care of a player who ran away: tells the room, then moves him through an exit chosen at random (he stays if there is none).
		 *
		 * @param player_id    id of the player
		 * @param enemy_id     id of the enemy he fled from
		 * @param requester_id the player who has his answer (NO_ACTOR if nobody has)
		 */
		void playerFled(long long player_id, const std::string& enemy_id, long long requester_id);

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Builds the manager of the fights.
		 *
		 * @param host the game, which must outlive the manager
		 */
		explicit CombatManager(CombatHost& host);

		//########################## COMMANDS ##########################
		/**
		 * Starts a fight with an enemy, joins the fight of this enemy, or plays an attack in the fight in progress.
		 *
		 * @param player_id id of the player
		 * @param target    the enemy to attack; empty (or the same enemy) during a fight
		 * @return          what happened, or why nothing did (no such npc, not hostile, another enemy, not his turn)
		 */
		CombatReport attack(long long player_id, const std::string& target);

		/**
		 * Plays a defense in the fight in progress.
		 *
		 * @param player_id id of the player
		 * @return          what happened, or why nothing did (not in combat, not his turn)
		 */
		CombatReport defend(long long player_id);

		/**
		 * Tries to run away from the fight in progress.
		 *
		 * @param player_id id of the player
		 * @return          what happened, or why nothing did (not in combat, not his turn)
		 */
		CombatReport flee(long long player_id);

		//########################## QUERIES ###########################
		/** Tells whether a player is in a fight. */
		bool inCombat(long long player_id) const;

		/**
		 * Describes the fight of a player for STATUS.
		 *
		 * @param player_id id of the player
		 * @return          the enemy, the round, whose turn it is and how long is left (nothing if he is not in a fight)
		 */
		std::optional<CombatView> view(long long player_id) const;

		//########################## UPDATES ###########################
		/**
		 * Takes a player out of his fight without a turn (he disconnects): the others go on.
		 *
		 * @param player_id id of the player
		 */
		void abandon(long long player_id);

		/**
		 * Plays a defense for every player who did not play in time.
		 *
		 * @param now the current time
		 */
		void update(std::chrono::steady_clock::time_point now);
};

#endif
