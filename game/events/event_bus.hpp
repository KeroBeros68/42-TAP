#ifndef EVENT_BUS_HPP
# define EVENT_BUS_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../shared/protocol/event.hpp"
# include "../world/room.hpp"

//##############################################################################
//#                                 EVENT BUS                                  #
//##############################################################################

class EventBus {
	private:
		//######################### VARIABLES ##########################
		std::vector<Event> _events;

	public:
		//########################## SENDING ###########################
		/**
		 * Queues an event for some players. Nothing is queued if there is no recipient.
		 *
		 * @param players_ids ids of the players who receive it
		 * @param line        the event line
		 */
		void send(const std::vector<long long>& players_ids, const std::string& line);

		/**
		 * Queues an event for every player of a room.
		 *
		 * @param room the room
		 * @param line the event line
		 */
		void toRoom(const Room& room, const std::string& line);

		/**
		 * Queues an event for every player of a room but one (who already has his answer).
		 *
		 * @param room      the room
		 * @param except_id id of the player who does not receive it
		 * @param line      the event line
		 */
		void toOthers(const Room& room, long long except_id, const std::string& line);

		//######################### COLLECTING #########################
		/**
		 * Takes the events that were queued.
		 *
		 * @return the events since the last call, in the order they were queued
		 */
		std::vector<Event> take();
};

#endif
