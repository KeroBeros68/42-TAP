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

// -- Poll slots ---------------------------------------------------------------
# define POLL_LISTEN_IDX	0	// slot of the listen socket in the poll array
# define POLL_SIGNAL_IDX	1	// slot of the signals in the poll array
# define POLL_CLIENT_START	2	// first slot of the clients in the poll array

#endif
