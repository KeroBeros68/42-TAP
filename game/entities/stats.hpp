#ifndef STATS_HPP
# define STATS_HPP

# include <algorithm>

# include "../../shared/defines.hpp"

# define COMBAT_INITIATIVE_MIN_PERCENT	50

enum class Status {
	HEALTHY,
	WOUNDED,
	COMBAT,
	DEAD
};

struct Stats {
	int max_hp = 0;
	int hp = 0;
	int attack = 0;
	int defense = 0;
	int hit = 0;
	int flee = 0;
	bool in_combat = false;

	Stats() = default;
	Stats(int max_hp, int hp, int attack, int defense, int hit, int flee)
		: max_hp(max_hp), hp(hp), attack(attack), defense(defense), hit(hit), flee(flee) {}

	bool isDead() const {
		return hp <= 0;
	}

	int initiative() const {
		if (max_hp <= 0)
			return flee;
		const int health_percent = COMBAT_INITIATIVE_MIN_PERCENT + (100 - COMBAT_INITIATIVE_MIN_PERCENT) * hp / max_hp;
		return flee * health_percent / 100;
	}

	Status status() const {
		if (isDead())
			return Status::DEAD;
		if (in_combat)
			return Status::COMBAT;
		if (hp < max_hp)
			return Status::WOUNDED;
		return Status::HEALTHY;
	}

	void takeDamage(int amount) {
		if (amount < 0)
			amount = 0;
		hp = std::max(0, hp - amount);
		if (isDead())
			in_combat = false;
	}

	void heal(int amount) {
		if (amount < 0 || isDead())
			return;
		hp = std::min(max_hp, hp + amount);
	}

	void revive(int new_hp) {
		hp = std::clamp(new_hp, 1, max_hp);
		in_combat = false;
	}
};

inline const char* statusName(Status status) {
	switch (status) {
		case Status::HEALTHY:	return STATUS_HEALTHY;
		case Status::WOUNDED:	return STATUS_WOUNDED;
		case Status::COMBAT:	return STATUS_COMBAT;
		case Status::DEAD:		return STATUS_DEAD;
	}
	return "healthy";
}

#endif
