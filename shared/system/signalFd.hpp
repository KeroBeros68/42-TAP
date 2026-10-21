#ifndef SIGNALFD_HPP
# define SIGNALFD_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <signal.h>
# include <sys/signalfd.h>
# include <unistd.h>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../defines.hpp"

//##############################################################################
//#                                 SIGNAL FD                                  #
//##############################################################################

class SignalFd {
	private:
		//######################### VARIABLES ##########################
		int _fd = INVALID_FD;
		sigset_t _mask;

		//######################## CONSTRUCTORS ########################
		/** Builds a SignalFd that watches nothing yet. */
		SignalFd(const SignalFd&);

		/** Copying is forbidden. */
		SignalFd& operator=(const SignalFd&);

	public:
		//######################## CONSTRUCTORS ########################
		/** Copying is forbidden. */
		SignalFd();

		/** Closes the descriptor. */
		~SignalFd();

		//######################### FUNCTIONS ##########################
		/**
		 * Adds a signal to watch (before open()).
		 *
		 * @param sig number of the signal
		 */
		void add(int sig);

		/**
		 * Blocks the watched signals for the whole process and starts to read them.
		 *
		 * @return true if it is open (also if it already was), false if it failed
		 */
		bool open();

		/** Returns the descriptor to give to poll, INVALID_FD while it is not open. */
		int fd() const;

		/**
		 * Reads one signal.
		 *
		 * @return the number of the signal, SIGNAL_NONE if none was waiting
		 */
		int read();
};

#endif
