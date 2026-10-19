//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <algorithm>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "combat.hpp"

//##############################################################################
//#                                  HELPERS                                   #
//##############################################################################

/** Tells whether a strike hits, with a roll below the chance to hit. */
static bool strikeHits(const Stats& attacker, const Stats& defender, double (*roll)()) {
	return roll() < hitChance(attacker, defender);
}

struct Strike {
	bool missed;
	int damage;
};

/**
 * Plays one strike: the defender loses hit points only if it hits.
 *
 * @param attacker who strikes
 * @param defender who is struck
 * @param roll     gives a number in [0, 1)
 * @param divisor  the damage is divided by it, rounded up (2 for a defending player)
 * @return         whether it missed, and the damage
 */
static Strike strike(const Stats& attacker, Stats& defender, double (*roll)(), int divisor = 1) {
	Strike result = {!strikeHits(attacker, defender, roll), 0};
	if (!result.missed)
		result.damage = (computeDamage(attacker, defender) + divisor - 1) / divisor;
	defender.takeDamage(result.damage);
	return result;
}

/** Tells whether the fight is over: the enemy is dead or no player is left. */
static bool isOver(const Encounter& encounter, const StatsOf& stats_of) {
	return encounter.participants.empty() || stats_of(ENEMY_ID)->isDead();
}

/** Begins a round: the highest initiative first, the players before the enemy (and in the order they joined) on a tie. */
static void beginRound(Encounter& encounter, const StatsOf& stats_of) {
	encounter.round++;
	encounter.order = encounter.participants;
	encounter.order.push_back(ENEMY_ID);
	std::stable_sort(encounter.order.begin(), encounter.order.end(), [&stats_of](long long a, long long b) {
		return stats_of(a)->initiative() > stats_of(b)->initiative();
	});
	encounter.turn = 0;
}

/** Records in the steps whether the enemy is dead and the fight over; then nobody is in combat any more. */
static void finish(const Encounter& encounter, const StatsOf& stats_of, Steps& steps) {
	steps.enemy_dead = stats_of(ENEMY_ID)->isDead();
	steps.over = isOver(encounter, stats_of);
	if (steps.over) {
		for (long long id : encounter.participants)
			stats_of(id)->in_combat = false;
		stats_of(ENEMY_ID)->in_combat = false;
	}
}

/**
 * Plays the turn of the enemy: it strikes a participant chosen with the roll.
 *
 * @param encounter the encounter
 * @param stats_of  gives the stats of a fighter
 * @param roll      gives a number in [0, 1)
 * @param steps     receives the turn, and the player if he dies
 */
static void enemyTurn(Encounter& encounter, const StatsOf& stats_of, double (*roll)(), Steps& steps) {
	encounter.turn++;
	const size_t count = encounter.participants.size();
	const long long target = encounter.participants[std::min(count - 1, static_cast<size_t>(roll() * count))];
	Stats& player = *stats_of(target);
	const Strike attack = strike(*stats_of(ENEMY_ID), player, roll, encounter.defending.count(target) ? COMBAT_DEFEND_DIVISOR : 1);
	steps.turns.push_back({ENEMY_ID, target, encounter.round, {false, CombatAction::ATTACK, attack.missed, attack.damage}});
	if (player.isDead()) {
		steps.dead.push_back(target);
		leave(encounter, target, stats_of);
	}
}

/**
 * Plays the turn of a player: his attack, his defense, or his escape.
 *
 * @param encounter the encounter
 * @param player_id the player who plays
 * @param action    what he does
 * @param stats_of  gives the stats of a fighter
 * @param roll      gives a number in [0, 1)
 * @param steps     receives the turn, and the player if he escapes
 */
static void playerTurn(Encounter& encounter, long long player_id, CombatAction action, const StatsOf& stats_of, double (*roll)(), Steps& steps) {
	encounter.turn++;
	encounter.defending.erase(player_id);
	Stats& player = *stats_of(player_id);
	Stats& enemy = *stats_of(ENEMY_ID);
	CombatEvent event = {true, action, false, 0};
	const int round = encounter.round;
	if (action == CombatAction::ATTACK) {
		const Strike attack = strike(player, enemy, roll);
		event.missed = attack.missed;
		event.damage = attack.damage;
	} else if (action == CombatAction::DEFEND) {
		encounter.defending.insert(player_id);
	} else {
		const bool escaped = roll() < escapeChance(player, enemy);
		event.missed = !escaped;
		if (escaped) {
			steps.escaped.push_back(player_id);
			leave(encounter, player_id, stats_of);
		}
	}
	steps.turns.push_back({player_id, ENEMY_ID, round, event});
}

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

int computeDamage(const Stats& attacker, const Stats& defender) {
	double damage = attacker.attack * DEFENSE_FACTOR(defender.defense);
	if (damage < 0)
		damage = 0;
	return std::max(COMBAT_MIN_DAMAGE, static_cast<int>(damage));
}

double hitChance(const Stats& attacker, const Stats& defender) {
	const double total = attacker.hit + COMBAT_FLEE_WEIGHT * defender.flee;
	if (total <= 0)
		return 1.0;
	return attacker.hit / total;
}

double escapeChance(const Stats& player, const Stats& enemy) {
	const double total = player.flee + enemy.hit;
	if (total <= 0)
		return 1.0;
	return player.flee / total;
}

Encounter startEncounter(const std::string& enemy_id) {
	Encounter encounter;
	encounter.enemy_id = enemy_id;
	return encounter;
}

bool isParticipant(const Encounter& encounter, long long player_id) {
	return std::find(encounter.participants.begin(), encounter.participants.end(), player_id) != encounter.participants.end();
}

void join(Encounter& encounter, long long player_id, const StatsOf& stats_of) {
	if (isParticipant(encounter, player_id))
		return;
	encounter.participants.push_back(player_id);
	if (encounter.round > 0)
		encounter.order.push_back(player_id);
	stats_of(player_id)->in_combat = true;
	stats_of(ENEMY_ID)->in_combat = true;
}

void leave(Encounter& encounter, long long player_id, const StatsOf& stats_of) {
	encounter.participants.erase(std::remove(encounter.participants.begin(), encounter.participants.end(), player_id), encounter.participants.end());
	encounter.defending.erase(player_id);
	const auto place = std::find(encounter.order.begin(), encounter.order.end(), player_id);
	if (place != encounter.order.end()) {
		if (static_cast<size_t>(place - encounter.order.begin()) < encounter.turn)
			encounter.turn--;
		encounter.order.erase(place);
	}
	if (Stats* stats = stats_of(player_id))
		stats->in_combat = false;
}

long long currentActor(Encounter& encounter, const StatsOf& stats_of) {
	if (isOver(encounter, stats_of))
		return NO_ACTOR;
	if (encounter.round == 0 || encounter.turn >= encounter.order.size())
		beginRound(encounter, stats_of);
	return encounter.order[encounter.turn];
}

long long peekActor(const Encounter& encounter) {
	if (encounter.round > 0 && encounter.turn < encounter.order.size())
		return encounter.order[encounter.turn];
	return NO_ACTOR;
}

Steps advance(Encounter& encounter, const StatsOf& stats_of, double (*roll)()) {
	Steps steps;
	while (currentActor(encounter, stats_of) == ENEMY_ID)
		enemyTurn(encounter, stats_of, roll, steps);
	finish(encounter, stats_of, steps);
	return steps;
}

Steps act(Encounter& encounter, long long player_id, CombatAction action, const StatsOf& stats_of, double (*roll)()) {
	Steps steps;
	if (currentActor(encounter, stats_of) != player_id) {
		finish(encounter, stats_of, steps);
		return steps;
	}
	playerTurn(encounter, player_id, action, stats_of, roll, steps);
	mergeSteps(steps, advance(encounter, stats_of, roll));
	return steps;
}

void mergeSteps(Steps& into, const Steps& more) {
	into.turns.insert(into.turns.end(), more.turns.begin(), more.turns.end());
	into.dead.insert(into.dead.end(), more.dead.begin(), more.dead.end());
	into.escaped.insert(into.escaped.end(), more.escaped.begin(), more.escaped.end());
	into.enemy_dead = more.enemy_dead;
	into.over = more.over;
}
