#ifndef CHAT_HPP
# define CHAT_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../defines.hpp"
# include "../text/utf8.hpp"

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Checks the text of a chat message.
 *
 * @param text the message, without the scope
 * @return     true if it is not empty, not longer than MAX_CHAT_LENGTH bytes, and valid UTF-8 without control characters
 */
inline bool isValidChatMessage(const std::string& text) {
	if (text.empty() || text.size() > MAX_CHAT_LENGTH)
		return false;
	return validateUTF8(text);
}

#endif
