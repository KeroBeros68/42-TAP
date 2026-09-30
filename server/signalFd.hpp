#ifndef SIGNALFD_HPP
# define SIGNALFD_HPP

# include <sys/signalfd.h>
# include <signal.h>
# include <unistd.h>

# include "defines.hpp"

class SignalFd {
	private:
		int _fd = INVALID_FD;
		sigset_t _mask;
		SignalFd(const SignalFd&);
		SignalFd& operator=(const SignalFd&);

	public:
		SignalFd();
		~SignalFd();

		void add(int sig);
		bool open();
		int fd() const;
		int read();
};

#endif