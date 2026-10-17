#ifndef LINE_HPP
# define LINE_HPP

# include <string>

# include "defines.hpp"

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

inline void splitCommand(const std::string& line, std::string& command, std::string& args) {
	const size_t separator = line.find(CMD_SEPARATOR);
	command = line.substr(0, separator);
	args = (separator == std::string::npos) ? "" : line.substr(separator + 1);
}

#endif
