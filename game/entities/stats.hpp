#ifndef STATS_HPP
# define STATS_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <algorithm>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../shared/defines.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Initiative ---------------------------------------------------------------
# define COMBAT_INITIATIVE_MIN_PERCENT	50	// initiative at 0 hit points, in percent of the agility

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class Status {
	HEALTHY,
	WOUNDED,
	COMBAT,
	DEAD
};

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct Stats {
	int max_hp = 0;
	int hp = 0;
	int attack = 0;
	int defense = 0;
	int hit = 0;
	int flee = 0;
	bool in_combat = false;

	/** Builds stats with every value at 0. */
	Stats() = default;

	/**
	 * Builds stats with the given values.
	 *
	 * @param max_hp  maximum hit points
	 * @param hp      current hit points
	 * @param attack  strength of the strikes
	 * @param defense resistance to the strikes
	 * @param hit     accuracy
	 * @param flee    evasion
	 */
	Stats(int max_hp, int hp, int attack, int defense, int hit, int flee)
		: max_hp(max_hp), hp(hp), attack(attack), defense(defense), hit(hit), flee(flee) {}

	/** Tells whether no hit point is left. */
	bool isDead() const {
		return hp <= 0;
	}

	/**
	 * Computes who acts first: the evasion, lowered by the wounds.
	 * It is the whole evasion at full hit points and COMBAT_INITIATIVE_MIN_PERCENT of it with almost none.
	 *
	 * @return the initiative, the highest acts first
	 */
	int initiative() const {
		if (max_hp <= 0)
			return flee;
		const int health_percent = COMBAT_INITIATIVE_MIN_PERCENT + (100 - COMBAT_INITIATIVE_MIN_PERCENT) * hp / max_hp;
		return flee * health_percent / 100;
	}

	/**
	 * Deduces the status from the hit points and the fight.
	 *
	 * @return dead, in combat, wounded or healthy, in this order of priority
	 */
	Status status() const {
		if (isDead())
			return Status::DEAD;
		if (in_combat)
			return Status::COMBAT;
		if (hp < max_hp)
			return Status::WOUNDED;
		return Status::HEALTHY;
	}

	/**
	 * Removes hit points (never below 0).
	 *
	 * @param amount the damage, a negative amount counts as 0
	 */
	void takeDamage(int amount) {
		if (amount < 0)
			amount = 0;
		hp = std::max(0, hp - amount);
		if (isDead())
			in_combat = false;
	}

	/**
	 * Gives hit points back, up to the maximum. A dead creature is not healed.
	 *
	 * @param amount the hit points, a negative amount does nothing
	 */
	void heal(int amount) {
		if (amount < 0 || isDead())
			return;
		hp = std::min(max_hp, hp + amount);
	}

	/**
	 * Brings a creature back to life, out of any fight.
	 *
	 * @param new_hp its hit points, kept between 1 and the maximum
	 */
	void revive(int new_hp) {
		hp = std::clamp(new_hp, 1, max_hp);
		in_combat = false;
	}
};

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/** Returns the word of the protocol for a status. */
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
