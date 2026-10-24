//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <cctype>
# include <cerrno>
# include <cstdlib>
# include <fstream>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../defines.hpp"
# include "loadenv.hpp"

//##############################################################################
//#                                  HELPERS                                   #
//##############################################################################

/**
 * Removes the spaces, the tabs and the line ends at both ends of a text.
 *
 * @param text the text
 * @return     the text without them
 */
static std::string trim(const std::string& text) {
	const std::string blanks = " \t\r\n";
	const size_t first = text.find_first_not_of(blanks);
	if (first == std::string::npos)
		return "";
	return text.substr(first, text.find_last_not_of(blanks) - first + 1);
}

/**
 * Removes a pair of quotes around a text, single or double.
 *
 * @param text the text
 * @return     the text inside the quotes, or the text itself if it has none
 */
static std::string unquote(const std::string& text) {
	if (text.size() >= 2 && (text.front() == '"' || text.front() == '\'') && text.back() == text.front())
		return text.substr(1, text.size() - 2);
	return text;
}

/**
 * Tells whether a text can be the name of a variable: letters, digits and underscores, and not empty.
 *
 * @param key the text
 * @return    true if it is a valid name
 */
static bool isValidKey(const std::string& key) {
	if (key.empty())
		return false;
	for (char c : key) {
		if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
			return false;
	}
	return true;
}

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

bool loadEnv(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open())
		return false;

	std::string line;
	while (std::getline(file, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;

		const size_t delimiter = line.find('=');
		if (delimiter == std::string::npos)
			continue;

		const std::string key = trim(line.substr(0, delimiter));
		const std::string value = unquote(trim(line.substr(delimiter + 1)));
		if (isValidKey(key))
			setenv(key.c_str(), value.c_str(), 0);
	}
	return true;
}

std::string getEnv(const char* name, const std::string& fallback) {
	if (!name)
		return fallback;
	const char* value = std::getenv(name);
	if (!value || !*value)
		return fallback;
	return value;
}

bool loadDefaultEnv() {
	const std::string candidates[] = {ENV_FILE, std::string("../") + ENV_FILE, std::string("../../") + ENV_FILE};
	for (const std::string& path : candidates) {
		if (loadEnv(path))
			return true;
	}
	return false;
}

long getEnvInt(const char* name, long fallback, long min, long max, bool* rejected) {
	const std::string text = getEnv(name);
	if (text.empty())
		return fallback;

	errno = 0;
	char* end = nullptr;
	const long value = std::strtol(text.c_str(), &end, 10);
	if (errno != 0 || *end != '\0' || value < min || value > max) {
		if (rejected)
			*rejected = true;
		return fallback;
	}
	return value;
}
