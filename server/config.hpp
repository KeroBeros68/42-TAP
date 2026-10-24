#ifndef CONFIG_HPP
# define CONFIG_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <cstddef>
# include <string>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../shared/defines.hpp"
# include "../shared/logging/logger.hpp"
# include "defines.hpp"

//##############################################################################
//#                                SERVER CONFIG                               #
//##############################################################################

struct ServConfig {
	//######################### VARIABLES ##########################
	long		port = SERVER_PORT;
	LogLevel	log_level = LogLevel::Debug;
	std::string	log_file;

	size_t	max_out_buffer = MAX_OUT_BUFFER;
	size_t	max_read_per_turn = MAX_READ_PER_TURN;
	int		command_window_ms = COMMAND_WINDOW_MS;
	int		max_commands_per_window = MAX_COMMANDS_PER_WINDOW;
	int		max_flood_windows = MAX_FLOOD_WINDOWS;
	size_t	max_sessions = MAX_SESSIONS;
	size_t	max_sessions_per_ip = MAX_SESSIONS_PER_IP;
	int		connect_window_ms = CONNECT_WINDOW_MS;
	size_t	max_connects_per_window = MAX_CONNECTS_PER_WINDOW;

	std::vector<std::string>	ignored;

	//######################### FUNCTIONS ##########################
	/**
	 * Reads the settings of the server: the .env file, then the environment (which wins).
	 * A setting that is missing keeps its default value. A setting that is not a number, or out of its bounds,
	 * keeps its default value too, and is listed in "ignored" as NAME=value.
	 * For the limits of commands, clients and connections, 0 turns the limit off.
	 */
	void loadServerConfig();
};

#endif
