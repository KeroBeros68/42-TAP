//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "config.hpp"
# include "../shared/system/loadenv.hpp"

//##############################################################################
//#                                SERVER CONFIG                               #
//##############################################################################

//######################### FUNCTIONS ##########################

void ServConfig::loadServerConfig() {
	loadDefaultEnv();

	port = getEnvInt(ENV_PORT, SERVER_PORT, MIN_PORT, MAX_PORT);

	log_level = logLevelFromName(getEnv(ENV_LOG_LEVEL), LogLevel::Debug);
	log_file = getEnv(ENV_LOG_FILE);

	max_out_buffer = getEnvInt(ENV_MAX_OUT_BUFFER, MAX_OUT_BUFFER, MIN_OUT_BUFFER, MAX_OUT_BUFFER_LIMIT);
	max_read_per_turn = getEnvInt(ENV_MAX_READ_PER_TURN, MAX_READ_PER_TURN, MIN_READ_PER_TURN, MAX_READ_PER_TURN_LIMIT);
	command_window_ms = getEnvInt(ENV_COMMAND_WINDOW_MS, COMMAND_WINDOW_MS, MIN_WINDOW_MS, MAX_WINDOW_MS);
	max_commands_per_window = getEnvInt(ENV_MAX_COMMANDS_PER_WINDOW, MAX_COMMANDS_PER_WINDOW, MIN_COUNT, MAX_COUNT);
	max_flood_windows = getEnvInt(ENV_MAX_FLOOD_WINDOWS, MAX_FLOOD_WINDOWS, MIN_COUNT, MAX_COUNT);
	max_sessions = getEnvInt(ENV_MAX_SESSIONS, MAX_SESSIONS, MIN_COUNT, MAX_COUNT);
	max_sessions_per_ip = getEnvInt(ENV_MAX_SESSIONS_PER_IP, MAX_SESSIONS_PER_IP, MIN_COUNT, MAX_COUNT);
	connect_window_ms = getEnvInt(ENV_CONNECT_WINDOW_MS, CONNECT_WINDOW_MS, MIN_WINDOW_MS, MAX_WINDOW_MS);
	max_connects_per_window = getEnvInt(ENV_MAX_CONNECTS_PER_WINDOW, MAX_CONNECTS_PER_WINDOW, MIN_COUNT, MAX_COUNT);
}
