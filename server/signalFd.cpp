#include "signalFd.hpp"

SignalFd::SignalFd() : _fd(INVALID_FD) { sigemptyset(&_mask);}

SignalFd::~SignalFd() {
	if (_fd != INVALID_FD)
		close(_fd);
}

void SignalFd::add(int sig) {
	sigaddset(&_mask, sig);
}

bool SignalFd::open() {
	if (_fd != INVALID_FD)
		return _fd;
	if (sigprocmask(SIG_BLOCK, &_mask, NULL) == SYSCALL_ERROR)
		return false;
	_fd = signalfd(INVALID_FD, &_mask, SFD_NONBLOCK | SFD_CLOEXEC);
	return _fd != INVALID_FD;
}

int SignalFd::fd() const {
	return _fd;
}

int SignalFd::read() {
	struct signalfd_siginfo si;
	ssize_t n = ::read(_fd, &si, sizeof(si));
	return (n == sizeof(si)) ? (int)si.ssi_signo : SIGNAL_NONE;
}