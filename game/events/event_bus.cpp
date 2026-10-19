//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <utility>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "event_bus.hpp"

//##############################################################################
//#                                 EVENT BUS                                  #
//##############################################################################

//########################## SENDING ###########################

void EventBus::send(const std::vector<long long>& players_ids, const std::string& line) {
	if (!players_ids.empty())
		_events.push_back(Event{players_ids, line});
}

void EventBus::toRoom(const Room& room, const std::string& line) {
	send(room.currentPlayer(), line);
}

void EventBus::toOthers(const Room& room, long long except_id, const std::string& line) {
	std::vector<long long> others;
	for (long long id : room.currentPlayer()) {
		if (id != except_id)
			others.push_back(id);
	}
	send(others, line);
}

//######################### COLLECTING #########################

std::vector<Event> EventBus::take() {
	std::vector<Event> taken;
	std::swap(taken, _events);
	return taken;
}
