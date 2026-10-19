#ifndef COMBAT_HPP
# define COMBAT_HPP

# include <string>
# include <vector>

# include "../../shared/random.hpp"
# include "../entities/stats.hpp"

# define COMBAT_DEFENSE_SCALE	50.0
# define DEFENSE_FACTOR(defense) (COMBAT_DEFENSE_SCALE / (COMBAT_DEFENSE_SCALE + (defense)))

# define COMBAT_MIN_DAMAGE		1

# define COMBAT_FLEE_WEIGHT		0.2

# define COMBAT_DEFEND_DIVISOR	2

int computeDamage(const Stats& attacker, const Stats& defender);
double hitChance(const Stats& attacker, const Stats& defender);

enum class CombatAction { ATTACK, DEFEND, FLEE };

struct CombatEvent {
	bool by_player;			// false: the enemy
	CombatAction action;
	bool missed;			// ATTACK: the strike missed. FLEE: the escape failed
	int damage;				// ATTACK: damage dealt to the other one
};

struct Combat {
	std::string enemy_id;
	int round = 1;
};

struct RoundOutcome {
	std::vector<CombatEvent> events;
	bool player_dead = false;
	bool enemy_dead = false;
	bool escaped = false;
};

Combat startCombat(const std::string& enemy_id);

double escapeChance(const Stats& player, const Stats& enemy);

RoundOutcome playRound(Combat& combat, CombatAction action, Stats& player, Stats& enemy, double (*roll)() = randomUnit);

#endif
