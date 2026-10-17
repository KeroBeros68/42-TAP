#ifndef PLAYER_NAME_HPP
# define PLAYER_NAME_HPP

# include <string>

# include "defines.hpp"
# include "utf8.hpp"

inline bool isValidPlayerName(const std::string& name) {
	if (name.empty() || name.size() > MAX_NAME_LENGTH)
		return false;
	if (!validateUTF8(name))
		return false;
	return name.find(CMD_SEPARATOR) == std::string::npos;
}

#endif
