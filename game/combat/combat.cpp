#include "combat.hpp"

# include <algorithm>

int computeDamage(const Stats& attacker, const Stats& defender) {
	int damage = attacker.attack * (100.0 / (100.0 + defender.defense));
	if (damage < 0)
		damage = 0;
	return std::max(1, static_cast<int>(damage));
}

CombatResult fight(Stats& attacker, Stats& defender) {
	CombatResult result = {AttackStatus::DONE, 0, 0, attacker.hp, defender.hp, false, false};

	if (attacker.isDead() || defender.isDead()) {
		result.status = AttackStatus::DEAD;
		result.attacker_status = attacker.status();
		return result;
	}

	attacker.in_combat = true;
	defender.in_combat = true;

	result.damage_to_defender = computeDamage(attacker, defender);
	defender.takeDamage(result.damage_to_defender);
	result.defender_hp = defender.hp;
	if (defender.isDead()) {
		attacker.in_combat = false;
		result.defender_dead = true;
		result.attacker_status = attacker.status();
		return result;
	}
	result.damage_to_attacker = computeDamage(defender, attacker);
	attacker.takeDamage(result.damage_to_attacker);
	result.attacker_hp = attacker.hp;
	result.attacker_dead = attacker.isDead();
	result.defender_dead = defender.isDead();
	result.attacker_status = attacker.status();

	return result;
}