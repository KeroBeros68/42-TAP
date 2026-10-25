//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "server.hpp"
# include "../../shared/protocol/line.hpp"
# include "../../shared/text/utf8.hpp"

//##############################################################################
//#                                  HELPERS                                   #
//##############################################################################

/**
 * Puts a socket in non-blocking mode; the error is logged.
 *
 * @param fd   the socket
 * @param what name of the socket, for the log
 * @return     false if it failed
 */
static bool setNonBlocking(int fd, const char* what) {
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == SYSCALL_ERROR) {
		LOG_ERROR(std::string("failed to set ") + what + " to non-blocking mode", {"error", strerror(errno)});
		return false;
	}
	return true;
}

/**
 * Refuses a new client: tells him why with one error line (without waiting) and closes his socket.
 *
 * @param fd the socket of the client
 */
static void refuse(int fd) {
	const std::string line = tapErrorLine(TapError::SERVER_BUSY) + LINE_END;
	::send(fd, line.data(), line.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
	close(fd);
}

/**
 * Finds the name of a user for the logs.
 *
 * @param user the user
 * @return     his name, or an empty text before CONNECT
 */
static std::string playerName(const User& user) {
	return user.isAuthenticated() ? user.name() : std::string();
}

/**
 * Cuts a long text for the logs, without cutting a UTF-8 character.
 *
 * @param text the text
 * @param max  the longest length, in bytes
 * @return     the text, or its beginning followed by "..."
 */
static std::string shorten(const std::string& text, size_t max) {
	const size_t cut = utf8SafeLength(text, max);
	return cut == text.size() ? text : text.substr(0, cut) + "...";
}

//##############################################################################
//#                                  SESSION                                   #
//##############################################################################

//######################## CONSTRUCTOR #########################

Server::Session::Session(int fd, const std::string& ip, long long id) : conn(fd, ip), user(id) {}

//##############################################################################
//#                                   SERVER                                   #
//##############################################################################

//######################## CONSTRUCTORS ########################

Server::Server(const ServConfig& config) : _config(config) {}

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

//############################# IO #############################

void Server::readFrom(Session& s) {
	Connection& c = s.conn;
	char buff[RECV_BUFFER_SIZE];
	size_t total = 0;

	while (total < _config.max_read_per_turn) {
		ssize_t n = ::recv(c.fd(), buff, sizeof(buff), 0);
		if (n > 0) {
			c.appendIn(buff, n);
			total += n;
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
	std::string line;
	while (c.popLine(line)) {
		if (line.size() > MAX_LINE_LENGTH) {
			LOG_WARN("line too long, disconnecting", {"client", s.user.id()}, {"ip", c.ip()}, {"bytes", line.size()});
			c.setClosing(true);
			replyError(c, TapError::BAD_REQUEST);
			return;
		}
		handleLine(s, line);
	}
	if (c.in().size() > MAX_LINE_LENGTH) {
		LOG_WARN("line too long, disconnecting", {"client", s.user.id()}, {"ip", c.ip()}, {"bytes", c.in().size()});
		c.setClosing(true);
		replyError(c, TapError::BAD_REQUEST);
	}
}

bool Server::allowCommand(Session& s) {
	if (_config.max_commands_per_window == 0)
		return true;
	const auto now = std::chrono::steady_clock::now();
	if (now - s.window_start >= std::chrono::milliseconds(_config.command_window_ms)) {
		s.flood_windows = s.window_flooded ? s.flood_windows : 0;
		s.window_start = now;
		s.window_commands = 0;
		s.window_flooded = false;
	}
	if (++s.window_commands <= _config.max_commands_per_window)
		return true;
	if (!s.window_flooded) {
		s.window_flooded = true;
		++s.flood_windows;
		LOG_WARN("command flooding", {"client", s.user.id()}, {"ip", s.conn.ip()}, {"player", playerName(s.user)}, {"windows", s.flood_windows});
		if (s.flood_windows >= _config.max_flood_windows) {
			LOG_WARN("flooding goes on, disconnecting", {"client", s.user.id()}, {"ip", s.conn.ip()});
			s.conn.setClosing(true);
		}
	}
	return false;
}

bool Server::allowConnection(const std::string& ip) {
	const auto now = std::chrono::steady_clock::now();
	for (auto it = _recent_connects.begin(); it != _recent_connects.end();) {
		std::deque<std::chrono::steady_clock::time_point>& times = it->second;
		while (!times.empty() && now - times.front() >= std::chrono::milliseconds(_config.connect_window_ms))
			times.pop_front();
		it = times.empty() ? _recent_connects.erase(it) : std::next(it);
	}

	size_t same_ip = 0;
	for (const auto& session : _sessions) {
		if (session.second.conn.ip() == ip)
			++same_ip;
	}
	if (_config.max_sessions && _sessions.size() >= _config.max_sessions) {
		LOG_WARN("too many clients, refusing a connection", {"ip", ip}, {"clients", _sessions.size()});
		return false;
	}
	if (_config.max_sessions_per_ip && same_ip >= _config.max_sessions_per_ip) {
		LOG_WARN("too many clients for one address, refusing a connection", {"ip", ip}, {"clients", same_ip});
		return false;
	}
	std::deque<std::chrono::steady_clock::time_point>& times = _recent_connects[ip];
	if (_config.max_connects_per_window && times.size() >= _config.max_connects_per_window) {
		LOG_WARN("connections too fast, refusing a connection", {"ip", ip}, {"connections", times.size()});
		return false;
	}
	times.push_back(now);
	return true;
}

void Server::handleLine(Session& s, const std::string& line)
{
	if (!allowCommand(s)) {
		replyError(s.conn, TapError::RATE_LIMITED);
		return;
	}
	std::string cmd, args;
	splitCommand(line, cmd, args);

	std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) {
        return std::toupper(c);
    });


	LOG_INFO("command received", {"client", s.user.id()}, {"player", playerName(s.user)}, {"cmd", cmd}, {"args", args});

	if (!s.user.isAuthenticated() && cmd != CMD_CONNECT && cmd != CMD_QUIT) {
		LOG_WARN("command before CONNECT", {"client", s.user.id()}, {"ip", s.conn.ip()}, {"cmd", cmd});
		replyError(s.conn, TapError::BAD_REQUEST);
		return;
	}
	auto it = _actions.find(cmd);
	if (it != _actions.end())
	{
		Response r = runAction(it->second, s, args);
		LOG_INFO("response sent", {"client", s.user.id()}, {"player", playerName(s.user)}, {"ok", r.isOk},
			{"response", r.isOk ? shorten(r.line(), LOG_RESPONSE_MAX) : r.line()});
		reply(s.conn, r.line());
		if (r.close)
			s.conn.setClosing(true);
	}
	else
	{
		LOG_WARN("unknown command", {"client", s.user.id()}, {"ip", s.conn.ip()}, {"cmd", cmd});
		replyError(s.conn, TapError::BAD_REQUEST);
	}
}

Response Server::runAction(const std::function<Response(User&, const std::string&)>& action, Session& s, const std::string& args) {
	try {
		return action(s.user, args);
	} catch (const std::exception& e) {
		LOG_ERROR("command failed", {"client", s.user.id()}, {"player", playerName(s.user)}, {"error", e.what()});
	} catch (...) {
		LOG_ERROR("command failed", {"client", s.user.id()}, {"player", playerName(s.user)}, {"error", "unknown exception"});
	}
	return Response::failure(TapError::INTERNAL_ERROR);
}

void Server::reply(Connection& c, const std::string& line) {
	c.appendOut(line + LINE_END);
	if (c.out().size() > _config.max_out_buffer && !c.isClosing()) {
		LOG_WARN("client does not read, disconnecting", {"ip", c.ip()}, {"bytes", c.out().size()});
		c.setClosing(true);
	}
}

void Server::replyError(Connection& c, TapError error) {
	reply(c, tapErrorLine(error));
}

void Server::flush(Connection& c) {
	while (!c.out().empty()) {
		ssize_t n = ::send(c.fd(), c.out().data(), c.out().size(), MSG_NOSIGNAL);
		if (n > 0) {
			c.consumeOut(n);
			continue;
		}
		if (n == SYSCALL_ERROR && (errno == EAGAIN || errno == EWOULDBLOCK))
			break;
		LOG_WARN("send failed", {"ip", c.ip()}, {"error", strerror(errno)});
		c.setClosing(true);
		break;
	}
}

//######################### LIFECYCLE ##########################

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
	server_addr.sin_port = htons(static_cast<uint16_t>(port));

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
	if (!setNonBlocking(_listen_socket, "listen socket"))
		throw std::runtime_error("Failed to set listen socket to non-blocking mode");
	LOG_DEBUG("listen socket set to non-blocking mode");

	_sigs.add(SIGINT);
	_sigs.add(SIGTERM);
	_sigs.add(SIGQUIT);
	if (!_sigs.open())
		throw std::runtime_error("Failed to open signalfd");
}

void Server::updatePoll() {
	for (auto it = _sessions.begin(); it != _sessions.end();) {
		if (it->second.conn.isClosing()) {
			try {
				if (_onDisconnect)
					_onDisconnect(it->second.user);
			} catch (const std::exception& e) {
				LOG_ERROR("disconnect callback failed", {"client", it->first}, {"error", e.what()});
			}
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
	if (ret == SYSCALL_ERROR) {
		if (errno != EINTR)
			LOG_ERROR("poll failed", {"error", strerror(errno)});
		return;
	}

	if (ret == 0) {
		return;
	}

	if (fds[POLL_SIGNAL_IDX].revents & POLLIN) {
		int sig = _sigs.read();
		if (sig == SIGINT || sig == SIGTERM || sig == SIGQUIT) {
			LOG_INFO("signal received, shutting down", {"signal", sig});
			_running = false;
		}
	}

	if (fds[POLL_LISTEN_IDX].revents & POLLIN) {
		sockaddr_in client_addr{};
		socklen_t client_len = sizeof(client_addr);
		int client_socket = accept(_listen_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
		if (client_socket == SYSCALL_ERROR) {
			LOG_ERROR("accept failed", {"error", strerror(errno)});
		} else {
			char ip[INET_ADDRSTRLEN] = UNKNOWN_IP;
			inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
			if (!allowConnection(ip))
				refuse(client_socket);
			else if (!setNonBlocking(client_socket, "client socket"))
				close(client_socket);
			else {
				long long client_id;
				if (_available_id.empty())
					client_id = _next_client_id++;
				else {
					client_id = _available_id.back();
					_available_id.pop_back();
				}
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

bool Server::getStatus() const {
	return _running;
}

//########################## COMMANDS ##########################

void Server::defineAction(const std::string& type, const std::function<Response(User&, const std::string&)>& action) {
	LOG_DEBUG("action defined", {"cmd", type});
	_actions[type] = action;
}

void Server::setOnDisconnect(const std::function<void(User&)>& callback) {
	_onDisconnect = callback;
}

//######################### MESSAGING ##########################

void Server::sendTo(const std::string& message, long long client_id) {
	auto it = _sessions.find(client_id);
	if (it != _sessions.end()) {
		LOG_DEBUG("sending message", {"client", client_id});
		reply(it->second.conn, message);
	} else {
		LOG_WARN("client not found", {"client", client_id});
	}
}

void Server::sendToArray(const std::string& message, const std::vector<long long>& clientIDs) {
	for (long long client_id : clientIDs) {
		sendTo(message, client_id);
	}
}

void Server::sendToAll(const std::string& message) {
	for (auto& session : _sessions) {
		reply(session.second.conn, message);
	}
}

void Server::flushAll() {
	for (auto& session : _sessions)
		flush(session.second.conn);
}

void Server::disconnect(long long client_id) {
	auto it = _sessions.find(client_id);
	if (it != _sessions.end())
		it->second.conn.setClosing(true);
}
