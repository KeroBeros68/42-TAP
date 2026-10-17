#include "combat.hpp"

# include <algorithm>

int computeDamage(const Stats& attacker, const Stats& defender) {
	int damage = attacker.attack * DEFENSE_FACTOR(defender.defense);
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

static bool strikeHits(const Stats& attacker, const Stats& defender, double (*roll)()) {
	return roll() < hitChance(attacker, defender);
}

struct Strike {
	bool missed;
	int damage;
};

static Strike strike(const Stats& attacker, Stats& defender, double (*roll)()) {
	Strike result = {!strikeHits(attacker, defender, roll), 0};
	if (!result.missed)
		result.damage = computeDamage(attacker, defender);
	defender.takeDamage(result.damage);
	return result;
}

CombatResult fight(Stats& attacker, Stats& defender, double (*roll)()) {
	CombatResult result = {AttackStatus::DONE, 0, 0, 0, 0, false, false};

	if (attacker.isDead() || defender.isDead()) {
		result.status = AttackStatus::DEAD;
	} else {
		attacker.in_combat = true;
		defender.in_combat = true;

		const Strike attack = strike(attacker, defender, roll);
		result.defender_missed = attack.missed;
		result.damage_to_defender = attack.damage;
		result.defender_dead = defender.isDead();

		if (result.defender_dead) {
			attacker.in_combat = false;
		} else {
			const Strike counter = strike(defender, attacker, roll);
			result.attacker_missed = counter.missed;
			result.damage_to_attacker = counter.damage;
			result.attacker_dead = attacker.isDead();
		}
	}

	result.attacker_hp = attacker.hp;
	result.defender_hp = defender.hp;
	result.attacker_status = attacker.status();
	return result;
}
