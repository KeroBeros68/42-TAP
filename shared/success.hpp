#ifndef SUCCESS_HPP
# define SUCCESS_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "defines.hpp"

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class TapOk {
	HELLO,
	PLAIN,
	DATA,
	CONNECTED,
	BYE,
	ROOM,
	PLAYERS,
	GROUP,
	TAKEN,
	DROPPED
};

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/** Returns the words that start the line of a success, like "room=". */
inline const char* tapOkPrefix(TapOk r) {
	switch (r) {
		case TapOk::HELLO:		return "hello proto=" PROTOCOL_VERSION;
		case TapOk::PLAIN:		return "";
		case TapOk::DATA:		return "";
		case TapOk::CONNECTED:	return "connected";
		case TapOk::BYE:		return "bye";
		case TapOk::ROOM:		return "room=";
		case TapOk::PLAYERS:	return "players=";
		case TapOk::GROUP:		return "group=";
		case TapOk::TAKEN:		return "taken=";
		case TapOk::DROPPED:	return "dropped=";
	}
	return "";
}

/**
 * Builds the line sent for a success.
 *
 * @param r       kind of success
 * @param payload text added after the words of the kind
 * @return        the line, like "OK room=loc.square" (without the line end)
 */
inline std::string tapOkLine(TapOk r, const std::string& payload = "") {
	std::string line = RESPONSE_OK;
	std::string body = std::string(tapOkPrefix(r)) + payload;
	if (!body.empty())
		line += " " + body;
	return line;
}

#endif
