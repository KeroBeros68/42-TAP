#ifndef DEFINES_HPP
# define DEFINES_HPP

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../shared/defines.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Logs and clients ---------------------------------------------------------
# define LOG_RESPONSE_MAX	100			// longest response written in the logs, in bytes
# define UNKNOWN_IP			"unknown"	// address written when the one of a client is not known

// -- Abuse limits -------------------------------------------------------------
# define MAX_OUT_BUFFER			65536	// bytes waiting to be sent to a client before he is disconnected
# define MAX_READ_PER_TURN		16384	// bytes read from a client in one turn of the loop
# define COMMAND_WINDOW_MS		1000	// length of the window in which the commands of a client are counted
# define MAX_COMMANDS_PER_WINDOW	20		// commands accepted from a client in one window
# define MAX_FLOOD_WINDOWS		3		// windows in a row over the limit before the client is disconnected
# define MAX_SESSIONS			64		// clients connected at the same time
# define MAX_SESSIONS_PER_IP	5		// clients connected at the same time from one address
# define CONNECT_WINDOW_MS		10000	// length of the window in which the connections of an address are counted
# define MAX_CONNECTS_PER_WINDOW	10		// connections accepted from one address in one window

// -- Bounds of the abuse limits (what the settings of the environment may take) --
# define MIN_OUT_BUFFER			1024		// lowest MAX_OUT_BUFFER
# define MAX_OUT_BUFFER_LIMIT	16777216	// highest MAX_OUT_BUFFER
# define MIN_READ_PER_TURN		1024		// lowest MAX_READ_PER_TURN
# define MAX_READ_PER_TURN_LIMIT	1048576		// highest MAX_READ_PER_TURN
# define MIN_WINDOW_MS			100			// lowest length of a window
# define MAX_WINDOW_MS			600000		// highest length of a window
# define MIN_COUNT				1			// lowest number of windows in a row
# define MIN_LIMIT				0			// lowest limit of commands, clients or connections: 0 turns the limit off
# define MAX_COUNT				10000		// highest number of commands, windows, clients or connections

// -- Poll slots ---------------------------------------------------------------
# define POLL_LISTEN_IDX	0	// slot of the listen socket in the poll array
# define POLL_SIGNAL_IDX	1	// slot of the signals in the poll array
# define POLL_CLIENT_START	2	// first slot of the clients in the poll array

#endif
