#ifndef LISTED_ITEM_HPP
# define LISTED_ITEM_HPP

# include <string>

/**
 * One entry of a widget list: the items on the floor, the inventory, the npcs.
 *
 * A list is a vector and not a map keyed by the id: the server now sends the
 * definition id ("item.herbs") and not an instance id ("item.herbs#3"), so two
 * herbs have the same id and a map would silently merge them into one entry.
 */
struct ListedItem {
	std::string	id;		// what the server expects back in TAKE, DROP, QUEST...
	std::string	name;	// what the player reads
};

#endif
