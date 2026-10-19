#ifndef PLAYER_NAME_HPP
# define PLAYER_NAME_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "defines.hpp"
# include "utf8.hpp"

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Checks the name that a player chooses.
 *
 * @param name the name given to CONNECT
 * @return     true if it is not empty, not longer than MAX_NAME_LENGTH, valid UTF-8 without control characters, and has no space
 */
inline bool isValidPlayerName(const std::string& name) {
	if (name.empty() || name.size() > MAX_NAME_LENGTH)
		return false;
	if (!validateUTF8(name))
		return false;
	return name.find(CMD_SEPARATOR) == std::string::npos;
}

#endif
