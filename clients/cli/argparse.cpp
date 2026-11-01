//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <arpa/inet.h>
# include <cerrno>
# include <cstdlib>
# include <stdexcept>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "argparse.hpp"
# include "../defines.hpp"
# include "../../shared/system/loadenv.hpp"

//##############################################################################
//#                                  HELPERS                                   #
//##############################################################################

/**
 * Reads a port: a whole number from MIN_PORT to MAX_PORT.
 *
 * @param text what was typed
 * @return     the port
 * @throws std::invalid_argument if it is not a port
 */
static int parsePort(const std::string& text) {
	errno = 0;
	char* end = nullptr;
	const long value = std::strtol(text.c_str(), &end, 10);
	if (text.empty() || errno != 0 || *end != '\0' || value < MIN_PORT || value > MAX_PORT)
		throw std::invalid_argument("invalid port '" + text + "' (expected a number from " + std::to_string(MIN_PORT)
			+ " to " + std::to_string(MAX_PORT) + ")");
	return static_cast<int>(value);
}

/**
 * Checks a host: the client connects with an IPv4 address, it does not resolve names.
 *
 * @param text what was typed
 * @return     the host
 * @throws std::invalid_argument if it is not an IPv4 address
 */
static std::string parseHost(const std::string& text) {
	in_addr address{};
	if (inet_pton(AF_INET, text.c_str(), &address) != 1)
		throw std::invalid_argument("invalid host '" + text + "' (expected an IPv4 address like 127.0.0.1)");
	return text;
}

//##############################################################################
//#                                  ARG PARSE                                 #
//##############################################################################

//######################## CONSTRUCTOR #########################

ArgParse::ArgParse(int argc, char** argv)
	: hostname(getEnv(ENV_IP, SERVER_IP)), port(static_cast<int>(getEnvInt(ENV_PORT, SERVER_PORT, MIN_PORT, MAX_PORT))) {
	for (int i = 1; i < argc; ++i) {
		const std::string arg = argv[i];
		if (arg == ARG_HELP || arg == ARG_HELP_SHORT) {
			help = true;
			return;
		}

		std::string name = arg;
		std::string value;
		bool has_value = false;
		const size_t equal = arg.find('=');
		if (arg.compare(0, 2, "--") == 0 && equal != std::string::npos) {
			name = arg.substr(0, equal);
			value = arg.substr(equal + 1);
			has_value = true;
		}
		if (name == ARG_HOST_SHORT)
			name = ARG_HOST;
		else if (name == ARG_PORT_SHORT)
			name = ARG_PORT;
		if (name != ARG_HOST && name != ARG_PORT)
			throw std::invalid_argument("unknown argument '" + arg + "'");
		if (!has_value) {
			if (i + 1 >= argc)
				throw std::invalid_argument("missing value for " + name);
			value = argv[++i];
		}

		if (name == ARG_HOST)
			hostname = parseHost(value);
		else
			port = parsePort(value);
	}
}

//######################### FUNCTIONS ##########################

std::string ArgParse::usage() {
	return std::string("Usage: clientCLI [") + ARG_HOST + " <ipv4>] [" + ARG_PORT + " <" + std::to_string(MIN_PORT) + "-"
		+ std::to_string(MAX_PORT) + ">] [" + ARG_HELP + "]\n"
		+ "  " + ARG_HOST_SHORT + ", " + ARG_HOST + "  address of the server (default: " + ENV_IP + " in the environment or .env, else "
		+ SERVER_IP + ")\n"
		+ "  " + ARG_PORT_SHORT + ", " + ARG_PORT + "  port of the server (default: " + ENV_PORT + " in the environment or .env, else "
		+ std::to_string(SERVER_PORT) + ")\n"
		+ "  " + ARG_HELP_SHORT + ", " + ARG_HELP + "  show this text\n";
}
