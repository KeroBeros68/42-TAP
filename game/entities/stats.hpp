#ifndef STATS_HPP
# define STATS_HPP

# include <algorithm>

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
	bool in_combat = false;

	Stats() = default;
	Stats(int max_hp, int hp, int attack, int defense)
		: max_hp(max_hp), hp(hp), attack(attack), defense(defense) {}

	bool isDead() const {
		return hp <= 0;
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
		case Status::HEALTHY:	return "healthy";
		case Status::WOUNDED:	return "wounded";
		case Status::COMBAT:	return "combat";
		case Status::DEAD:		return "dead";
	}
	return "healthy";
}

#endif
