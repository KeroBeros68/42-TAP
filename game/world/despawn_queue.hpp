#ifndef DESPAWN_QUEUE_HPP
# define DESPAWN_QUEUE_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <chrono>
# include <map>
# include <string>
# include <unordered_map>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../shared/logging/logger.hpp"
# include "world.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Despawn ------------------------------------------------------------------
# define ITEM_DESPAWN_SECONDS	120	// time an item stays on the floor after it was dropped, before it disappears

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct Despawn {
	std::string room_id;
	std::string item_id;
};

//##############################################################################
//#                              DESPAWN QUEUE                                 #
//##############################################################################

class DespawnQueue {
	private:
		//######################### VARIABLES ##########################
		std::multimap<std::chrono::steady_clock::time_point, Despawn> _queue;
		std::unordered_map<std::string, std::chrono::steady_clock::time_point> _deadlines;

	public:
		//######################### SCHEDULING #########################
		/**
		 * Plans the disappearance of an item that lies on the floor of a room, after ITEM_DESPAWN_SECONDS.
		 * An item that is dropped again later gets a new delay.
		 *
		 * @param room_id id of the room
		 * @param item_id instance id of the item
		 * @param now     the current time
		 */
		void schedule(const std::string& room_id, const std::string& item_id, std::chrono::steady_clock::time_point now);

		//########################## UPDATES ###########################
		/**
		 * Removes the items whose time came, if they are still on the floor where they were planned.
		 *
		 * @param now        the current time
		 * @param world      the world, for the rooms
		 * @param on_despawn called as on_despawn(room, definition id) for each item removed
		 */
		template <typename OnDespawn>
		void update(std::chrono::steady_clock::time_point now, World& world, OnDespawn on_despawn) {
			while (!_queue.empty() && _queue.begin()->first <= now) {
				const auto due = _queue.begin();
				const Despawn despawn = due->second;
				const bool current = _deadlines.count(despawn.item_id) && _deadlines.at(despawn.item_id) == due->first;
				_queue.erase(due);
				if (!current)
					continue;
				_deadlines.erase(despawn.item_id);

				auto room = world.rooms.find(despawn.room_id);
				if (room == world.rooms.end())
					continue;
				const Item* item = room->second.inventory().findItem(despawn.item_id);
				if (!item || item->id() != despawn.item_id)
					continue;
				const std::string definition_id = item->definition().id();
				room->second.inventory().remove(despawn.item_id);
				LOG_INFO("item despawned", {"room", despawn.room_id}, {"item", despawn.item_id});
				on_despawn(room->second, definition_id);
			}
		}
};

#endif
