#ifndef STATS_HPP
# define STATS_HPP

// Combat statistics of a creature (NPC or player).
// Add the new stats here (with a default value), one field at a time.
struct Stats {
	int hp = 0;
	int attack = 0;
	int defense = 0;
};

#endif
