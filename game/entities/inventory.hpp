#ifndef INVENTORY_HPP
# define INVENTORY_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <optional>
# include <string>
# include <unordered_map>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "item.hpp"

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class TransferStatus { Moved, NotFound, DestinationFull, NotTakeable };

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct TransferResult {
	TransferStatus	status;
	std::string		id;
	std::string		definition_id = "";
};

//##############################################################################
//#                                 INVENTORY                                  #
//##############################################################################

class Inventory {
	private:
		//######################### VARIABLES ##########################
		std::unordered_map<std::string, Item> _items;
		size_t _capacity;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Builds an empty inventory.
		 *
		 * @param capacity number of items it can hold, 0 for no limit
		 */
		explicit Inventory(size_t capacity = 0);

		//########################## QUERIES ###########################
		/**
		 * Finds the item that a query designates.
		 *
		 * @param query instance id, definition id, name or one word of the name, in any case
		 * @return      the item (the first by id if several match), nullptr if there is none
		 */
		const Item* findItem(const std::string& query) const;

		/**
		 * Same as the const version, but gives an item that can be modified.
		 *
		 * @param query instance id, definition id, name or one word of the name, in any case
		 * @return      the item (the first by id if several match), nullptr if there is none
		 */
		Item* findItem(const std::string& query);

		/** Tells whether the capacity is reached (never for an inventory with no limit). */
		bool isFull() const;

		/** Lists the instance ids of the items, in alphabetical order. */
		std::vector<std::string> ids() const;

		/** Lists the ids of the definitions of the items, in alphabetical order (twice if there are two of them). */
		std::vector<std::string> definitionIds() const;

		/** Returns all the items, by instance id. */
		const std::unordered_map<std::string, Item>& items() const;

		/** Returns the number of items held. */
		size_t size() const;

		//########################## CHANGES ###########################
		/**
		 * Puts an item in the inventory.
		 *
		 * @param item the item to add
		 * @return     false if the inventory is full or already holds an item with this id
		 */
		bool add(const Item& item);

		/**
		 * Takes an item out of the inventory.
		 *
		 * @param id instance id of the item
		 * @return   the item, or nothing if there is no item with this id
		 */
		std::optional<Item> remove(const std::string& id);
};

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Moves the item that a query designates from an inventory to another one.
 * Nothing moves if the item is not found or if the destination is full.
 *
 * @param from  inventory that gives the item
 * @param to    inventory that receives it
 * @param query designates the item, as for Inventory::findItem
 * @return      the status of the move, and the id of the item and of its definition (empty if nothing moved)
 */
TransferResult transfer(Inventory& from, Inventory& to, const std::string& query);

#endif
