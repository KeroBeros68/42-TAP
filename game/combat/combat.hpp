#ifndef COMBAT_HPP
# define COMBAT_HPP

# include "../../shared/random.hpp"
# include "../entities/stats.hpp"

// Part of the attack that goes through the defense of the target: with a defense of 100, the damage is halved
# define DEFENSE_FACTOR(defense) (100.0 / (100.0 + (defense)))

enum class AttackStatus{
	DONE,
	NOT_FOUND,
	NOT_HOSTILE,
	DEAD
};

struct CombatResult {
	AttackStatus	status;
	int damage_to_defender;
	int damage_to_attacker;
	int attacker_hp;
	int defender_hp;
	bool attacker_dead;
	bool defender_dead;
	Status attacker_status = Status::HEALTHY;	// status of the attacker after the round
	bool attacker_missed = false;				// the counter-attack missed
	bool defender_missed = false;				// the attack missed
};

int computeDamage(const Stats& attacker, const Stats& defender);
// Chance (0 to 1) that the attacker hits the defender: hit / (hit + COMBAT_FLEE_WEIGHT * flee)
double hitChance(const Stats& attacker, const Stats& defender);
// One round: the attacker strikes, then the defender strikes back if it is still alive.
// A strike hits if roll() < hitChance (a number in [0, 1)); the roll can be replaced to test.
CombatResult fight(Stats& attacker, Stats& defender, double (*roll)() = randomUnit);

#endif
