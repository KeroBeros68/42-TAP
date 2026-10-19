#ifndef EVENT_HPP
# define EVENT_HPP

# include <vector>
# include <string>

# include "defines.hpp"

# define EVT_ROOM			"ROOM"
# define EVT_STATS			"STATS"
# define EVT_PRESENCE_ENTER	"PRESENCE ENTER"
# define EVT_PRESENCE_LEAVE	"PRESENCE LEAVE"
# define EVT_PLAYERS_FIELD	"players="

struct Event {
	std::vector<long long> players_ids;
	std::string line;
};

inline std::string eventLine(const std::string& category, const std::string& data) {
	return std::string(RESPONSE_EVT) + CMD_SEPARATOR + category + CMD_SEPARATOR + data;
}

#endif
