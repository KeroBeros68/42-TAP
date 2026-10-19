#ifndef LINE_HPP
# define LINE_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "defines.hpp"

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Takes the first complete line out of a receive buffer.
 *
 * @param buffer the bytes received so far: the line and its line end are removed from it
 * @param line   receives the line, without its line end and its carriage return
 * @return       true if a line was taken, false (and nothing changes) if the buffer holds none yet
 */
inline bool extractLine(std::string& buffer, std::string& line) {
	const size_t pos = buffer.find(LINE_END);
	if (pos == std::string::npos)
		return false;
	line = buffer.substr(0, pos);
	buffer.erase(0, pos + 1);
	if (!line.empty() && line.back() == CARRIAGE_RETURN)
		line.pop_back();
	return true;
}

/**
 * Splits a line into its command and its arguments, at the first separator.
 *
 * @param line    a line received, like "MOVE north"
 * @param command receives the first word
 * @param args    receives the rest of the line, empty if there is none
 */
inline void splitCommand(const std::string& line, std::string& command, std::string& args) {
	const size_t separator = line.find(CMD_SEPARATOR);
	command = line.substr(0, separator);
	args = (separator == std::string::npos) ? "" : line.substr(separator + 1);
}

#endif
