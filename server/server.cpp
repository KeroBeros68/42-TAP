#include "server.hpp"


Server::Session::Session(int fd, const std::string& ip, long long id) : conn(fd, ip), user(id) {}

Server::Server() {}

Server::~Server() {
	if (_listen_socket != INVALID_FD) {
		std::cout << "Closing listen socket..." << std::endl;
		if (close(_listen_socket) == SYSCALL_ERROR) {
			std::cerr << "Failed to close listen socket: " << strerror(errno) << std::endl;
		}
		std::cout << "Listen socket closed successfully" << std::endl;
	}
	for (const auto& session : _sessions) {
		std::cout << "Closing client socket..." << std::endl;
		if (close(session.second.conn.fd()) == SYSCALL_ERROR) {
			std::cerr << "Failed to close client socket: " << strerror(errno) << std::endl;
		}
		std::cout << "Client socket closed successfully" << std::endl;
	}
}

void Server::start(const size_t& port) {
	std::cout << "Starting server on port " << port << std::endl;

	_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listen_socket == INVALID_FD) {
		std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
		throw std::runtime_error("Failed to create socket");
	}

	std::cout << "Socket created successfully" << std::endl;

	std::cout << "Setting socket options..." << std::endl;
	int opt = 1;
	if (setsockopt(_listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == SYSCALL_ERROR) {
		std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
		throw std::runtime_error("Failed to set socket options");
	}

	std::cout << "Socket options set successfully" << std::endl;

	std::cout << "Binding socket to port " << port << "..." << std::endl;
	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(_listen_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SYSCALL_ERROR) {
		std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
		throw std::runtime_error("Failed to bind socket");
	}

	std::cout << "Socket bound successfully" << std::endl;

	std::cout << "Listening for incoming connections..." << std::endl;
	if (listen(_listen_socket, SOMAXCONN) == SYSCALL_ERROR) {
		std::cerr << "Failed to listen on socket: " << strerror(errno) << std::endl;
		throw std::runtime_error("Failed to listen on socket");
	}

	std::cout << "Server started successfully on port " << port << std::endl;

	std::cout << "Setting listen socket to non-blocking mode..." << std::endl;
	if (fcntl(_listen_socket, F_SETFL, O_NONBLOCK) == SYSCALL_ERROR) {
		std::cerr << "Failed to set listen socket to non-blocking mode: " << strerror(errno) << std::endl;
		throw std::runtime_error("Failed to set listen socket to non-blocking mode");
	}
	std::cout << "Listen socket set to non-blocking mode successfully" << std::endl;

	_sigs.add(SIGINT);
	_sigs.add(SIGTERM);
	if (!_sigs.open())
		throw std::runtime_error("Failed to open signalfd");
}

void Server::defineAction(const std::string& type, const std::function<Response(User&, const std::string&)>& action) {
	std::cout << "Defining action for message type: " << type << std::endl;
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
		std::cout << "Sending message to client ID " << client_id << std::endl;
		reply(it->second.conn, message);
		flush(it->second.conn);
	} else {
		std::cerr << "Client ID " << client_id << " not found" << std::endl;
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
			std::cout << "Client socket closed for client: " << it->first << std::endl;
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
			std::cerr << "Poll error: " << strerror(errno) << std::endl;
		return;
	}

	if (ret == 0) {
		return;
	}

	if (fds[POLL_SIGNAL_IDX].revents & POLLIN) {
		int sig = _sigs.read();
		if (sig == SIGINT || sig == SIGTERM)
			_running = false;
	}

	if (fds[POLL_LISTEN_IDX].revents & POLLIN) {
		sockaddr_in client_addr{};
		socklen_t client_len = sizeof(client_addr);
		int client_socket = accept(_listen_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
		if (client_socket < 0) {
			std::cerr << "Accept error: " << strerror(errno) << std::endl;
		} else {
			if (fcntl(client_socket, F_SETFL, O_NONBLOCK) == SYSCALL_ERROR) {
				std::cerr << "Failed to set client socket to non-blocking mode: " << strerror(errno) << std::endl;
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
				std::cout << "New client connected with ID: " << client_id << " (" << ip << ")" << std::endl;
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
			c.setClosing(true);
			break;
		}
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			break;
		if (errno == EINTR)
			continue;

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
		c.setClosing(true);
		reply(c, tapErrorLine(TapError::BAD_REQUEST));
	}
}

void Server::handeLine(Session& s, const std::string& line)
{
	size_t sp = line.find(CMD_SEPARATOR);
	std::string cmd = line.substr(0, sp);
	std::string args = (sp == std::string::npos) ? "" : line.substr(sp + 1);

	if (!s.user.isAuthenticated() && cmd != CMD_CONNECT) {
		reply(s.conn, tapErrorLine(TapError::BAD_REQUEST));
		return;
	}
	auto it = _actions.find(cmd);
	if (it != _actions.end())
	{
		Response r = it->second(s.user, args);
		reply(s.conn, r.line());
		if (r.close)
			s.conn.setClosing(true);
	}
	else
	{
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
        c.setClosing(true); break;
    }
}

bool Server::getStatus() const {
	return _running;
}