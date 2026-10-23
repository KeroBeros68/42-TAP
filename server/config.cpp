//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "config.hpp"
# include "../shared/system/loadenv.hpp"

//##############################################################################
//#                                SERVER CONFIG                               #
//##############################################################################

//##############################################################################
//#                                  HELPERS                                   #
//##############################################################################

/**
 * Reads a numeric setting; if it is refused, it is noted in the list of the ignored settings.
 *
 * @param name     name of the variable
 * @param fallback default value
 * @param min      lowest value accepted
 * @param max      highest value accepted
 * @param ignored  receives "NAME=value" if the setting is refused
 * @return         the value, or the default value
 */
static long readSetting(const char* name, long fallback, long min, long max, std::vector<std::string>& ignored) {
	bool rejected = false;
	const long value = getEnvInt(name, fallback, min, max, &rejected);
	if (rejected)
		ignored.push_back(std::string(name) + "=" + getEnv(name));
	return value;
}

//##############################################################################
//#                                SERVER CONFIG                               #
//##############################################################################

//######################### FUNCTIONS ##########################

void ServConfig::loadServerConfig() {
	loadDefaultEnv();
	ignored.clear();

	port = readSetting(ENV_PORT, SERVER_PORT, MIN_PORT, MAX_PORT, ignored);

	log_level = logLevelFromName(getEnv(ENV_LOG_LEVEL), LogLevel::Debug);
	log_file = getEnv(ENV_LOG_FILE);

	max_out_buffer = readSetting(ENV_MAX_OUT_BUFFER, MAX_OUT_BUFFER, MIN_OUT_BUFFER, MAX_OUT_BUFFER_LIMIT, ignored);
	max_read_per_turn = readSetting(ENV_MAX_READ_PER_TURN, MAX_READ_PER_TURN, MIN_READ_PER_TURN, MAX_READ_PER_TURN_LIMIT, ignored);
	command_window_ms = readSetting(ENV_COMMAND_WINDOW_MS, COMMAND_WINDOW_MS, MIN_WINDOW_MS, MAX_WINDOW_MS, ignored);
	max_commands_per_window = readSetting(ENV_MAX_COMMANDS_PER_WINDOW, MAX_COMMANDS_PER_WINDOW, MIN_LIMIT, MAX_COUNT, ignored);
	max_flood_windows = readSetting(ENV_MAX_FLOOD_WINDOWS, MAX_FLOOD_WINDOWS, MIN_COUNT, MAX_COUNT, ignored);
	max_sessions = readSetting(ENV_MAX_SESSIONS, MAX_SESSIONS, MIN_LIMIT, MAX_COUNT, ignored);
	max_sessions_per_ip = readSetting(ENV_MAX_SESSIONS_PER_IP, MAX_SESSIONS_PER_IP, MIN_LIMIT, MAX_COUNT, ignored);
	connect_window_ms = readSetting(ENV_CONNECT_WINDOW_MS, CONNECT_WINDOW_MS, MIN_WINDOW_MS, MAX_WINDOW_MS, ignored);
	max_connects_per_window = readSetting(ENV_MAX_CONNECTS_PER_WINDOW, MAX_CONNECTS_PER_WINDOW, MIN_LIMIT, MAX_COUNT, ignored);
}
