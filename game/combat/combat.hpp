#ifndef COMBAT_HPP
# define COMBAT_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <functional>
# include <set>
# include <string>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../shared/defines.hpp"
# include "../../shared/system/random.hpp"
# include "../entities/stats.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Damage and hit chance ----------------------------------------------------
# define COMBAT_DEFENSE_SCALE	50.0															// with a defense of this value, the damage is halved
# define DEFENSE_FACTOR(defense) (COMBAT_DEFENSE_SCALE / (COMBAT_DEFENSE_SCALE + (defense)))	// part of an attack that goes through a defense
# define COMBAT_MIN_DAMAGE		1																// a strike that hits always does at least this damage
# define COMBAT_DEFEND_DIVISOR	2																// a defending player receives this much less damage
# define COMBAT_FLEE_WEIGHT		0.2																// weight of the flee against the hit in the chance to hit

// -- Encounter ----------------------------------------------------------------
# define ENEMY_ID				(-1)	// id of the enemy among the fighters (the ids of the players are >= 0)
# define NO_ACTOR				(-2)	// nobody has to play: the fight is over
# define COMBAT_TURN_SECONDS	20		// time to play before a defense is played for the player

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class CombatAction { ATTACK, DEFEND, FLEE };

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct CombatEvent {
	bool by_player;
	CombatAction action;
	bool missed;
	int damage;
};

struct Turn {
	long long actor;
	long long target;
	int round;
	CombatEvent event;
};

struct Steps {
	std::vector<Turn> turns;
	std::vector<long long> dead;
	std::vector<long long> escaped;
	bool enemy_dead = false;
	bool over = false;
};

struct Encounter {
	std::string enemy_id;
	int round = 0;
	std::vector<long long> participants;
	std::vector<long long> order;
	size_t turn = 0;
	std::set<long long> defending;
};

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

typedef std::function<Stats*(long long)> StatsOf;

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/** Returns the word of the protocol for an action. */
inline const char* actionName(CombatAction action) {
	switch (action) {
		case CombatAction::ATTACK:	return ACTION_ATTACK;
		case CombatAction::DEFEND:	return ACTION_DEFEND;
		case CombatAction::FLEE:	return ACTION_FLEE;
	}
	return "";
}

/**
 * Computes the damage of a strike that hits: the attack, lowered by the defense.
 *
 * @param attacker who strikes
 * @param defender who is struck
 * @return         the damage, at least COMBAT_MIN_DAMAGE
 */
int computeDamage(const Stats& attacker, const Stats& defender);

/**
 * Computes the chance that a strike hits: hit / (hit + COMBAT_FLEE_WEIGHT * flee).
 *
 * @param attacker who strikes
 * @param defender who is struck
 * @return         a chance between 0 and 1 (1 if there is no hit and no flee)
 */
double hitChance(const Stats& attacker, const Stats& defender);

/**
 * Computes the chance that a player escapes: flee / (flee + hit of the enemy).
 *
 * @param player who tries to flee
 * @param enemy  who is fought
 * @return       a chance between 0 and 1 (1 if there is no flee and no hit)
 */
double escapeChance(const Stats& player, const Stats& enemy);

/**
 * Creates the fight against an enemy, with nobody in it yet.
 *
 * @param enemy_id id of the enemy
 * @return         the encounter
 */
Encounter startEncounter(const std::string& enemy_id);

/** Tells whether a player is in the encounter. */
bool isParticipant(const Encounter& encounter, long long player_id);

/**
 * Adds a player to the encounter: he plays at the end of the round in progress, and he and the enemy are in combat.
 *
 * @param encounter the encounter
 * @param player_id id of the player
 * @param stats_of  gives the stats of a fighter
 */
void join(Encounter& encounter, long long player_id, const StatsOf& stats_of);

/**
 * Takes a player out of the encounter: he is not in combat any more.
 *
 * @param encounter the encounter
 * @param player_id id of the player
 * @param stats_of  gives the stats of a fighter
 */
void leave(Encounter& encounter, long long player_id, const StatsOf& stats_of);

/**
 * Tells whose turn it is, and begins a new round (order of the initiative) if the previous one is over.
 *
 * @param encounter the encounter
 * @param stats_of  gives the stats of a fighter
 * @return          ENEMY_ID, the id of a player, or NO_ACTOR if the fight is over
 */
long long currentActor(Encounter& encounter, const StatsOf& stats_of);

/**
 * Tells whose turn it is, without beginning a round.
 *
 * @param encounter the encounter
 * @return          ENEMY_ID, the id of a player, or NO_ACTOR
 */
long long peekActor(const Encounter& encounter);

/**
 * Plays the turns of the enemy until it is the turn of a player, or the fight is over.
 *
 * @param encounter the encounter
 * @param stats_of  gives the stats of a fighter
 * @param roll      gives a number in [0, 1) for each draw (to be replaced in a test)
 * @return          the turns that were played, and who died or left
 */
Steps advance(Encounter& encounter, const StatsOf& stats_of, double (*roll)() = randomUnit);

/**
 * Plays the turn of a player, then the turns that follow it. Nothing happens if it is not his turn.
 *
 * @param encounter the encounter
 * @param player_id the player who plays
 * @param action    what he does
 * @param stats_of  gives the stats of a fighter
 * @param roll      gives a number in [0, 1) for each draw (to be replaced in a test)
 * @return          the turns that were played, and who died or left
 */
Steps act(Encounter& encounter, long long player_id, CombatAction action, const StatsOf& stats_of, double (*roll)() = randomUnit);

/**
 * Adds what happened in some turns to what happened before.
 *
 * @param into receives the turns, the deaths and the escapes
 * @param more what happened afterwards
 */
void mergeSteps(Steps& into, const Steps& more);

#endif
