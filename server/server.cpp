#include "server.hpp"

static std::string playerName(const User& user);


Server::Session::Session(int fd, const std::string& ip, long long id) : conn(fd, ip), user(id) {}

Server::Server() {}

Server::~Server() {
	if (_listen_socket != INVALID_FD) {
		LOG_DEBUG("closing listen socket");
		if (close(_listen_socket) == SYSCALL_ERROR) {
			LOG_ERROR("failed to close listen socket", {"error", strerror(errno)});
		}
		LOG_DEBUG("listen socket closed");
	}
	for (const auto& session : _sessions) {
		LOG_DEBUG("closing client socket");
		if (close(session.second.conn.fd()) == SYSCALL_ERROR) {
			LOG_ERROR("failed to close client socket", {"error", strerror(errno)});
		}
		LOG_DEBUG("client socket closed");
	}
}

void Server::start(const size_t& port) {
	LOG_INFO("starting server", {"port", port});

	_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listen_socket == INVALID_FD) {
		LOG_ERROR("failed to create socket", {"error", strerror(errno)});
		throw std::runtime_error("Failed to create socket");
	}

	LOG_DEBUG("socket created");

	LOG_DEBUG("setting socket options");
	int opt = 1;
	if (setsockopt(_listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == SYSCALL_ERROR) {
		LOG_ERROR("failed to set socket options", {"error", strerror(errno)});
		throw std::runtime_error("Failed to set socket options");
	}

	LOG_DEBUG("socket options set");

	LOG_DEBUG("binding socket", {"port", port});
	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(_listen_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SYSCALL_ERROR) {
		LOG_ERROR("failed to bind socket", {"port", port}, {"error", strerror(errno)});
		throw std::runtime_error("Failed to bind socket");
	}

	LOG_DEBUG("socket bound");

	LOG_DEBUG("listening for incoming connections");
	if (listen(_listen_socket, SOMAXCONN) == SYSCALL_ERROR) {
		LOG_ERROR("failed to listen on socket", {"error", strerror(errno)});
		throw std::runtime_error("Failed to listen on socket");
	}

	LOG_INFO("server started", {"port", port});

	LOG_DEBUG("setting listen socket to non-blocking mode");
	if (fcntl(_listen_socket, F_SETFL, O_NONBLOCK) == SYSCALL_ERROR) {
		LOG_ERROR("failed to set listen socket to non-blocking mode", {"error", strerror(errno)});
		throw std::runtime_error("Failed to set listen socket to non-blocking mode");
	}
	LOG_DEBUG("listen socket set to non-blocking mode");

	_sigs.add(SIGINT);
	_sigs.add(SIGTERM);
	if (!_sigs.open())
		throw std::runtime_error("Failed to open signalfd");
}

void Server::defineAction(const std::string& type, const std::function<Response(User&, const std::string&)>& action) {
	LOG_DEBUG("action defined", {"cmd", type});
	_actions[type] = action;
}

void Server::setOnDisconnect(const std::function<void(User&)>& callback) {
	_onDisconnect = callback;
}

void Server::disconnect(long long client_id) {
	auto it = _sessions.find(client_id);
	if (it != _sessions.end())
		it->second.conn.setClosing(true);
}

void Server::sendTo(const std::string& message, long long client_id) {
	auto it = _sessions.find(client_id);
	if (it != _sessions.end()) {
		LOG_DEBUG("sending message", {"client", client_id});
		reply(it->second.conn, message);
		flush(it->second.conn);
	} else {
		LOG_WARN("client not found", {"client", client_id});
	}
}

void Server::sendToArray(const std::string& message, std::vector<long long> clientIDs) {
	for (long long client_id : clientIDs) {
		sendTo(message, client_id);
	}
}

void Server::sendToAll(const std::string& message) {
	for (auto& session : _sessions) {
		reply(session.second.conn, message);
		flush(session.second.conn);
	}
}

void Server::updatePoll() {
	for (auto it = _sessions.begin(); it != _sessions.end();) {
		if (it->second.conn.isClosing()) {
			if (_onDisconnect)
				_onDisconnect(it->second.user);
			close(it->second.conn.fd());
			LOG_INFO("client disconnected", {"client", it->first}, {"ip", it->second.conn.ip()}, {"player", playerName(it->second.user)});
			_available_id.push_back(it->first);
			it = _sessions.erase(it);
		} else {
			++it;
		}
	}

	std::vector<pollfd> fds(POLL_CLIENT_START);
	std::vector<long long> ids(POLL_CLIENT_START, INVALID_FD);
	fds[POLL_LISTEN_IDX].fd = _listen_socket;
	fds[POLL_LISTEN_IDX].events = POLLIN;
	fds[POLL_SIGNAL_IDX].fd = _sigs.fd();
	fds[POLL_SIGNAL_IDX].events = POLLIN;

	for (const auto& session : _sessions) {
		pollfd p{};
		p.fd = session.second.conn.fd();
		p.events = POLLIN | (session.second.conn.out().empty() ? 0 : POLLOUT);
		fds.push_back(p);
		ids.push_back(session.first);
	}

	int ret = poll(fds.data(), fds.size(), POLL_TIMEOUT_MS);
	if (ret < 0) {
		if (errno != EINTR)
			LOG_ERROR("poll failed", {"error", strerror(errno)});
		return;
	}

	if (ret == 0) {
		return;
	}

	if (fds[POLL_SIGNAL_IDX].revents & POLLIN) {
		int sig = _sigs.read();
		if (sig == SIGINT || sig == SIGTERM) {
			LOG_INFO("signal received, shutting down", {"signal", sig});
			_running = false;
		}
	}

	if (fds[POLL_LISTEN_IDX].revents & POLLIN) {
		sockaddr_in client_addr{};
		socklen_t client_len = sizeof(client_addr);
		int client_socket = accept(_listen_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
		if (client_socket < 0) {
			LOG_ERROR("accept failed", {"error", strerror(errno)});
		} else {
			if (fcntl(client_socket, F_SETFL, O_NONBLOCK) == SYSCALL_ERROR) {
				LOG_ERROR("failed to set client socket to non-blocking mode", {"error", strerror(errno)});
				close(client_socket);
			}
			else {
				long long client_id;
				if (_available_id.empty())
					client_id = _next_client_id++;
				else {
					client_id = _available_id.back();
					_available_id.pop_back();
				}
				char ip[INET_ADDRSTRLEN] = "unknown";
				inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
				Session& session = _sessions.emplace(client_id, Session(client_socket, ip, client_id)).first->second;
				LOG_INFO("client connected", {"client", client_id}, {"ip", ip});
				reply(session.conn, tapOkLine(TapOk::HELLO));
				flush(session.conn);
			}
		}
	}

	for (size_t i = POLL_CLIENT_START; i < fds.size(); i++) {
		auto it = _sessions.find(ids[i]);
		if (it == _sessions.end())
			continue;
		Session& session = it->second;
		if (fds[i].revents & (POLLIN | POLLHUP | POLLERR))
			readFrom(session);
		flush(session.conn);
	}
}

void Server::readFrom(Session& s) {
	Connection& c = s.conn;
	char buff[RECV_BUFFER_SIZE];

	while (true) {
		ssize_t n = ::recv(c.fd(), buff, sizeof(buff), 0);
		if (n > 0) {
			c.appendIn(buff, n);
			continue;
		}
		if (n == 0) {
			LOG_DEBUG("client closed the connection", {"client", s.user.id()});
			c.setClosing(true);
			break;
		}
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			break;
		if (errno == EINTR)
			continue;

		LOG_WARN("recv failed", {"client", s.user.id()}, {"error", strerror(errno)});
		c.setClosing(true);
		break;
	}
	size_t pos;
	while ((pos = c.in().find(LINE_END)) != std::string::npos) {
		std::string line = c.in().substr(0, pos);
		c.eraseIn(pos + 1);
		if (!line.empty() && line.back() == CARRIAGE_RETURN)
			line.pop_back();
		handeLine(s, line);
	}
	if (c.in().size() > MAX_LINE_LENGTH) {
		LOG_WARN("line too long, disconnecting", {"client", s.user.id()}, {"ip", c.ip()}, {"bytes", c.in().size()});
		c.setClosing(true);
		reply(c, tapErrorLine(TapError::BAD_REQUEST));
	}
}

// The name of an authenticated user, "" before CONNECT
static std::string playerName(const User& user) {
	return user.isAuthenticated() ? user.name() : std::string();
}

// The beginning of a long text (a LOOK response is about 600 bytes), never cut inside a UTF-8 character
static std::string shorten(const std::string& text, size_t max) {
	if (text.size() <= max)
		return text;
	size_t cut = max;
	while (cut > 0 && (static_cast<unsigned char>(text[cut]) & 0xC0) == 0x80)
		cut--;
	return text.substr(0, cut) + "...";
}

void Server::handeLine(Session& s, const std::string& line)
{
	size_t sp = line.find(CMD_SEPARATOR);
	std::string cmd = line.substr(0, sp);
	std::string args = (sp == std::string::npos) ? "" : line.substr(sp + 1);

	LOG_INFO("command received", {"client", s.user.id()}, {"player", playerName(s.user)}, {"cmd", cmd}, {"args", args});

	if (!s.user.isAuthenticated() && cmd != CMD_CONNECT) {
		LOG_WARN("command before CONNECT", {"client", s.user.id()}, {"ip", s.conn.ip()}, {"cmd", cmd});
		reply(s.conn, tapErrorLine(TapError::BAD_REQUEST));
		return;
	}
	auto it = _actions.find(cmd);
	if (it != _actions.end())
	{
		Response r = it->second(s.user, args);
		LOG_INFO("response sent", {"client", s.user.id()}, {"player", playerName(s.user)}, {"ok", r.isOk},
			{"response", r.isOk ? shorten(r.line(), 100) : r.line()});
		reply(s.conn, r.line());
		if (r.close)
			s.conn.setClosing(true);
	}
	else
	{
		LOG_WARN("unknown command", {"client", s.user.id()}, {"ip", s.conn.ip()}, {"cmd", cmd});
		reply(s.conn, tapErrorLine(TapError::BAD_REQUEST));
	}
}

void Server::reply(Connection& c, const std::string& line) {
	c.appendOut(line + LINE_END);
}

void Server::flush(Connection& c) {
    while (!c.out().empty()) {
        ssize_t n = ::send(c.fd(), c.out().data(), c.out().size(), MSG_NOSIGNAL);
        if (n > 0) {
			c.consumeOut(n);
			continue;
		}
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			break;
        LOG_WARN("send failed", {"ip", c.ip()}, {"error", strerror(errno)});
        c.setClosing(true); break;
    }
}

bool Server::getStatus() const {
	return _running;
}