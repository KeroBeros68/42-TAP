#include "combat.hpp"

# include <algorithm>

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

static bool strikeHits(const Stats& attacker, const Stats& defender, double (*roll)()) {
	return roll() < hitChance(attacker, defender);
}

struct Strike {
	bool missed;
	int damage;
};

static Strike strike(const Stats& attacker, Stats& defender, double (*roll)(), int divisor = 1) {
	Strike result = {!strikeHits(attacker, defender, roll), 0};
	if (!result.missed)
		result.damage = (computeDamage(attacker, defender) + divisor - 1) / divisor;
	defender.takeDamage(result.damage);
	return result;
}

Combat startCombat(const std::string& enemy_id) {
	Combat combat;
	combat.enemy_id = enemy_id;
	return combat;
}

double escapeChance(const Stats& player, const Stats& enemy) {
	const double total = player.flee + enemy.hit;
	if (total <= 0)
		return 1.0;
	return player.flee / total;
}

static CombatEvent playerTurn(CombatAction action, Stats& player, Stats& enemy, double (*roll)(), bool& escaped) {
	CombatEvent event = {true, action, false, 0};
	if (action == CombatAction::ATTACK) {
		const Strike attack = strike(player, enemy, roll);
		event.missed = attack.missed;
		event.damage = attack.damage;
	} else if (action == CombatAction::FLEE) {
		escaped = roll() < escapeChance(player, enemy);
		event.missed = !escaped;
	}
	return event;
}

static CombatEvent enemyTurn(bool player_defends, Stats& player, const Stats& enemy, double (*roll)()) {
	const Strike attack = strike(enemy, player, roll, player_defends ? COMBAT_DEFEND_DIVISOR : 1);
	return {false, CombatAction::ATTACK, attack.missed, attack.damage};
}

RoundOutcome playRound(Combat& combat, CombatAction action, Stats& player, Stats& enemy, double (*roll)()) {
	RoundOutcome outcome;
	const bool player_first = player.initiative() >= enemy.initiative();
	player.in_combat = true;
	enemy.in_combat = true;

	for (int turn = 0; turn < 2 && !outcome.escaped && !player.isDead() && !enemy.isDead(); turn++) {
		if ((turn == 0) == player_first)
			outcome.events.push_back(playerTurn(action, player, enemy, roll, outcome.escaped));
		else
			outcome.events.push_back(enemyTurn(action == CombatAction::DEFEND, player, enemy, roll));
	}

	outcome.player_dead = player.isDead();
	outcome.enemy_dead = enemy.isDead();
	if (outcome.escaped || outcome.player_dead || outcome.enemy_dead) {
		player.in_combat = false;
		enemy.in_combat = false;
	} else {
		combat.round++;
	}
	return outcome;
}
