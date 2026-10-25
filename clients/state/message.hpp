/*
	The ServerMessage structure is the data model for a server response.
	It is used by the Session to handle server responses.
*/

#ifndef MESSAGE_HPP
# define MESSAGE_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <optional>
# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../third_party/json.hpp"

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

/**
 * One line received from the server, already split into its parts.
 *
 * Everything the protocol says is read here; nothing is interpreted.
 * Fields carry a meaning depending on kind:
 *   OK  -> data
 *   ERR -> code, name
 *   EVT -> category, data
 */
struct ServerMessage {
	enum class Kind { OK, ERR, EVT, UNKNOWN };

	Kind kind = Kind::UNKNOWN;
	int code = 0;            // ERR only: the numeric code, 0 if absent
	std::string name;        // ERR only: the symbolic name, empty if absent
	std::string category;    // EVT only: "ROOM", "STATS", "GLOBAL", "GROUP"
	std::string data;        // the rest of the line, untouched
};

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Splits one server line into its parts.
 *
 * Never throws: a line that does not match the grammar comes back as UNKNOWN
 * with the whole line in data.
 *
 * @param line a line as received, without its line end
 * @return     the split message
 */
ServerMessage parseLine(const std::string& line);

/**
 * Reads a payload as JSON, only if it looks like JSON.
 *
 * @param data the payload of an OK or an EVT
 * @return     the parsed value, or nothing if data is not JSON
 */
std::optional<nlohmann::json> asJson(const std::string& data);

#endif
