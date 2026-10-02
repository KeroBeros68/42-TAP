#include "client.hpp"

Client::Client() : _fd(-1), _servername(SERVER_IP), _port(SERVER_PORT) {}

Client::~Client() {
	disconnect();
}

void Client::connect() {
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd == -1) {
		throw std::runtime_error("Failed to create socket");
	}

	struct sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(_port);
	server_addr.sin_addr.s_addr = inet_addr(_servername.c_str());

	if (::connect(_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		disconnect();
		throw std::runtime_error("Failed to connect to server");
	}
}

void Client::disconnect() {
	if (_fd != -1) {
		std::cout << "Disconnecting from server..." << std::endl;
		close(_fd);
		_fd = -1;
	}
}

bool Client::isConnected() const {
	return _fd != -1;
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

bool Client::update(int timeout_ms) {
	if (!isConnected())
		return false;

	pollfd fds[2] = {};
	fds[0].fd = _fd;
	fds[0].events = POLLIN;
	nfds_t count = 1;
	if (_keyboard) {
		fds[1].fd = STDIN_FILENO;
		fds[1].events = POLLIN;
		count = 2;
	}

	int ret = poll(fds, count, timeout_ms);
	if (ret < 0)
		return errno == EINTR;
	if (ret == 0)
		return true;

	if (fds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
		if (!readServer())
			return false;
	}
	if (_keyboard && (fds[1].revents & (POLLIN | POLLHUP))) {
		if (!readKeyboard())
			return false;
	}
	return true;
}

bool Client::readServer() {
	char buff[BUFFER_SIZE];
	ssize_t n = ::recv(_fd, buff, sizeof(buff), 0);
	if (n <= 0) {
		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
			return true;
		disconnect();
		return false;
	}
	_in.append(buff, n);

	size_t pos;
	while ((pos = _in.find(LINE_END)) != std::string::npos) {
		std::string line = _in.substr(0, pos);
		_in.erase(0, pos + 1);
		if (!line.empty() && line.back() == CARRIAGE_RETURN)
			line.pop_back();
		handleLine(line);
	}
	return true;
}

bool Client::readKeyboard() {
	char buff[BUFFER_SIZE];
	ssize_t n = ::read(STDIN_FILENO, buff, sizeof(buff));
	if (n <= 0) {
		if (n < 0 && (errno == EAGAIN || errno == EINTR))
			return true;
		return false;
	}
	_kbd.append(buff, n);

	size_t pos;
	while ((pos = _kbd.find(LINE_END)) != std::string::npos) {
		std::string line = _kbd.substr(0, pos);
		_kbd.erase(0, pos + 1);
		if (!line.empty() && line.back() == CARRIAGE_RETURN)
			line.pop_back();
		if (line.empty())
			continue;
		if (!send(line))
			return false;
	}
	return true;
}

void Client::handleLine(const std::string& line) {
	size_t sp = line.find(CMD_SEPARATOR);
	std::string type = line.substr(0, sp);
	std::string args = (sp == std::string::npos) ? "" : line.substr(sp + 1);

	auto it = _actions.find(type);
	if (it != _actions.end())
		it->second(args);
	else
		std::cerr << "No action defined for server message: " << line << std::endl;
}
