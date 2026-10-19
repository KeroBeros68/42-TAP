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

// -- Poll slots ---------------------------------------------------------------
# define POLL_LISTEN_IDX	0	// slot of the listen socket in the poll array
# define POLL_SIGNAL_IDX	1	// slot of the signals in the poll array
# define POLL_CLIENT_START	2	// first slot of the clients in the poll array

#endif
