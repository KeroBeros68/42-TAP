//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "respawn_queue.hpp"

//##############################################################################
//#                               RESPAWN QUEUE                                #
//##############################################################################

//######################### SCHEDULING #########################

bool RespawnQueue::schedule(const Respawn& respawn) {
	if (respawn.seconds <= 0)
		return false;
	_queue.emplace(std::chrono::steady_clock::now() + std::chrono::seconds(respawn.seconds), respawn);
	return true;
}
