#include "combat.hpp"

# include <algorithm>

# include "../../shared/defines.hpp"

int computeDamage(const Stats& attacker, const Stats& defender) {
	int damage = attacker.attack * DEFENSE_FACTOR(defender.defense);
	if (damage < 0)
		damage = 0;
	return std::max(1, static_cast<int>(damage));
}

double hitChance(const Stats& attacker, const Stats& defender) {
	const double total = attacker.hit + COMBAT_FLEE_WEIGHT * defender.flee;
	if (total <= 0)
		return 1.0;
	return attacker.hit / total;
}

static bool strikeHits(const Stats& attacker, const Stats& defender, double (*roll)()) {
	return roll() < hitChance(attacker, defender);
}

CombatResult fight(Stats& attacker, Stats& defender, double (*roll)()) {
	CombatResult result = {AttackStatus::DONE, 0, 0, attacker.hp, defender.hp, false, false};

	if (attacker.isDead() || defender.isDead()) {
		result.status = AttackStatus::DEAD;
		result.attacker_status = attacker.status();
		return result;
	}

	attacker.in_combat = true;
	defender.in_combat = true;

	result.defender_missed = !strikeHits(attacker, defender, roll);
	if (!result.defender_missed)
		result.damage_to_defender = computeDamage(attacker, defender);
	defender.takeDamage(result.damage_to_defender);
	result.defender_hp = defender.hp;
	if (defender.isDead()) {
		attacker.in_combat = false;
		result.defender_dead = true;
		result.attacker_status = attacker.status();
		return result;
	}
	result.attacker_missed = !strikeHits(defender, attacker, roll);
	if (!result.attacker_missed)
		result.damage_to_attacker = computeDamage(defender, attacker);
	attacker.takeDamage(result.damage_to_attacker);
	result.attacker_hp = attacker.hp;
	result.attacker_dead = attacker.isDead();
	result.defender_dead = defender.isDead();
	result.attacker_status = attacker.status();

	return result;
}