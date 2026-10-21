#include "client.hpp"
#include "../shared/protocol/line.hpp"
#include "../shared/system/loadenv.hpp"

Client::Client() : _fd(INVALID_FD), _servername(getEnv(ENV_IP, SERVER_IP)), _port(static_cast<int>(getEnvInt(ENV_PORT, SERVER_PORT, MIN_PORT, MAX_PORT))) {}

Client::~Client() {
	disconnect();
}

void Client::connect() {
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd == INVALID_FD) {
		throw std::runtime_error("Failed to create socket");
	}

	struct sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(_port);
	server_addr.sin_addr.s_addr = inet_addr(_servername.c_str());

	if (::connect(_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SYSCALL_ERROR) {
		disconnect();
		throw std::runtime_error("Failed to connect to server");
	}
}

void Client::disconnect() {
	if (_fd != INVALID_FD) {
		std::cout << "Disconnecting from server..." << std::endl;
		close(_fd);
		_fd = INVALID_FD;
	}
}

bool Client::isConnected() const {
	return _fd != INVALID_FD;
}

int Client::fd() const {
	return _fd;
}

bool Client::isAuthenticated() const {
	return _is_authenticated;
}

void Client::defineAction(const std::string& type, const std::function<void(const std::string&)>& action) {
	_actions[type] = action;
}

bool Client::send(const std::string& message) {
	if (!isConnected())
		return false;
	std::string data = message + LINE_END;
	size_t sent = 0;
	while (sent < data.size()) {
		ssize_t n = ::send(_fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
		if (n > 0) {
			sent += n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		disconnect();
		return false;
	}
	return true;
}

void Client::enableKeyboard(bool enable) {
	_keyboard = enable;
}

bool Client::enableSignals() {
	_sigs.add(SIGINT);
	_sigs.add(SIGTERM);
	_sigs.add(SIGQUIT);
	_signals = _sigs.open();
	return _signals;
}

void Client::quitOnSignal() {
	send(CMD_QUIT);
	pollfd server = {_fd, POLLIN, 0};
	while (isConnected() && poll(&server, 1, POLL_TIMEOUT_MS) > 0) {
		if (!readServer())
			break;
	}
}

bool Client::update(int timeout_ms) {
	if (!isConnected())
		return false;

	pollfd fds[3] = {};
	fds[0].fd = _fd;
	fds[0].events = POLLIN;
	nfds_t count = 1;
	int keyboard_idx = -1;
	int signal_idx = -1;
	if (_keyboard) {
		keyboard_idx = count++;
		fds[keyboard_idx].fd = STDIN_FILENO;
		fds[keyboard_idx].events = POLLIN;
	}
	if (_signals) {
		signal_idx = count++;
		fds[signal_idx].fd = _sigs.fd();
		fds[signal_idx].events = POLLIN;
	}

	int ret = poll(fds, count, timeout_ms);
	if (ret < 0)
		return errno == EINTR;
	if (ret == 0)
		return true;

	if (signal_idx != -1 && (fds[signal_idx].revents & POLLIN)) {
		_sigs.read();
		quitOnSignal();
		return false;
	}
	if (fds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
		if (!readServer())
			return false;
	}
	if (keyboard_idx != -1 && (fds[keyboard_idx].revents & (POLLIN | POLLHUP))) {
		if (!readKeyboard())
			return false;
	}
	return true;
}

bool Client::readServer() {
	char buff[RECV_BUFFER_SIZE];
	ssize_t n = ::recv(_fd, buff, sizeof(buff), 0);
	if (n <= 0) {
		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
			return true;
		disconnect();
		return false;
	}
	_in.append(buff, n);

	std::string line;
	while (extractLine(_in, line))
		handleLine(line);
	return true;
}

bool Client::readKeyboard() {
	char buff[RECV_BUFFER_SIZE];
	ssize_t n = ::read(STDIN_FILENO, buff, sizeof(buff));
	if (n <= 0) {
		if (n < 0 && (errno == EAGAIN || errno == EINTR))
			return true;
		return false;
	}
	_kbd.append(buff, n);

	std::string line;
	while (extractLine(_kbd, line)) {
		if (line.empty())
			continue;
		if (!send(line))
			return false;
	}
	return true;
}

void Client::handleLine(const std::string& line) {
	std::string type, args;
	splitCommand(line, type, args);

	auto it = _actions.find(type);
	if (it != _actions.end())
		it->second(args);
	else
		std::cerr << "No action defined for server message: " << line << std::endl;
}

void Client::setServer(const std::string& address, int port)
{
    _servername = address;
    _port = port;
}