//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "despawn_queue.hpp"

//##############################################################################
//#                              DESPAWN QUEUE                                 #
//##############################################################################

//######################### SCHEDULING #########################

void DespawnQueue::schedule(const std::string& room_id, const std::string& item_id, std::chrono::steady_clock::time_point now) {
	const auto deadline = now + std::chrono::seconds(ITEM_DESPAWN_SECONDS);
	_deadlines[item_id] = deadline;
	_queue.emplace(deadline, Despawn{room_id, item_id});
}
