#ifndef RESPAWN_QUEUE_HPP
# define RESPAWN_QUEUE_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <chrono>
# include <map>
# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../shared/protocol/event.hpp"
# include "../../shared/logging/logger.hpp"
# include "spawn.hpp"
# include "world.hpp"

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class RespawnKind { Npc, Item };

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct Respawn {
	RespawnKind kind;
	std::string def_id;
	std::string room_id;
	int seconds;

	/**
	 * Describes the respawn of an npc or of an item created by a spawn.
	 * Read it before the object leaves its place: it does not exist any more afterwards.
	 *
	 * @param kind    npc or item
	 * @param spawned the object, which remembers its spawn
	 * @return        the respawn, with a delay of 0 if the object does not come back
	 */
	template <typename Spawned>
	static Respawn of(RespawnKind kind, const Spawned& spawned) {
		return {kind, spawned.definition().id(), spawned.homeRoom(), spawned.respawnSeconds()};
	}
};

//##############################################################################
//#                               RESPAWN QUEUE                                #
//##############################################################################

class RespawnQueue {
	private:
		//######################### VARIABLES ##########################
		std::multimap<std::chrono::steady_clock::time_point, Respawn> _queue;

		//######################### FUNCTIONS ##########################
		/**
		 * Creates the object of a respawn and puts it in its room.
		 *
		 * @param database npcs or items, to create the object
		 * @param label    "npc" or "item", for the log
		 * @param respawn  what comes back
		 * @param room     the room where it is put
		 * @param place    puts the object in the room: place(room, object)
		 * @return         the id of the new object
		 */
		template <typename Db, typename Place>
		static std::string create(const Db& database, const char* label, const Respawn& respawn, Room& room, Place place) {
			const auto instance = spawnInstance(database, respawn.def_id, respawn.room_id, respawn.seconds, room, place);
			LOG_INFO(std::string(label) + " respawned", {"room", respawn.room_id}, {label, instance.id()});
			return instance.id();
		}

	public:
		//######################### SCHEDULING #########################
		/**
		 * Plans the respawn, after its own delay.
		 *
		 * @param respawn what has to come back
		 * @return        false (and nothing is planned) if it does not come back
		 */
		bool schedule(const Respawn& respawn);

		//########################## UPDATES ###########################
		/**
		 * Brings back what is due: creates it in its room, then tells it.
		 *
		 * @param now      the current time
		 * @param world    the world, for the rooms and the definitions
		 * @param on_spawn called as on_spawn(room, kind, id) for each one, with kind EVT_SPAWN_NPC or EVT_SPAWN_ITEM
		 */
		template <typename OnSpawn>
		void update(std::chrono::steady_clock::time_point now, World& world, OnSpawn on_spawn) {
			while (!_queue.empty() && _queue.begin()->first <= now) {
				const Respawn respawn = _queue.begin()->second;
				_queue.erase(_queue.begin());

				auto room = world.rooms.find(respawn.room_id);
				if (room == world.rooms.end()) {
					LOG_ERROR("respawn in an unknown room", {"room", respawn.room_id}, {"definition", respawn.def_id});
					continue;
				}
				if (respawn.kind == RespawnKind::Npc)
					on_spawn(room->second, EVT_SPAWN_NPC, create(world.npc_database, "npc", respawn, room->second,
						[](Room& r, const Npc& npc) { r.addNpc(npc); }));
				else
					on_spawn(room->second, EVT_SPAWN_ITEM, create(world.item_database, "item", respawn, room->second,
						[](Room& r, const Item& item) { r.inventory().add(item); }));
			}
		}
};

#endif
