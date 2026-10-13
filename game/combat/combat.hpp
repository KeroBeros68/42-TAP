#ifndef COMBAT_HPP
# define COMBAT_HPP

# include "../entities/stats.hpp"

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
};

int computeDamage(const Stats& attacker, const Stats& defender);
CombatResult fight(Stats& attacker, Stats& defender);

#endif
