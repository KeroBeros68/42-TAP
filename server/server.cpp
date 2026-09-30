#include "server.hpp"

Server::Client::Client(int fd, long long id) : fd(fd), id(id) {}

Server::Server() {}

Server::~Server() {
	if (_listen_socket != INVALID_FD) {
		std::cout << "Closing listen socket..." << std::endl;
		if (close(_listen_socket) == SYSCALL_ERROR) {
			std::cerr << "Failed to close listen socket: " << strerror(errno) << std::endl;
		}
		std::cout << "Listen socket closed successfully" << std::endl;
	}
	for (const auto& client : _clients) {
		std::cout << "Closing client socket..." << std::endl;
		if (close(client.second.fd) == SYSCALL_ERROR) {
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

void Server::defineAction(const std::string& type, const std::function<void(long long&, const std::string&)>& action) {
	std::cout << "Defining action for message type: " << type << std::endl;
	_actions[type] = action;
}

void Server::sendToSocket(int socket, const std::string& message) {
	std::vector<uint8_t> bytes(message.begin(), message.end());
	ssize_t sent_bytes = bytes.size();

	if (::send(socket, &sent_bytes, sizeof(sent_bytes), 0) == SYSCALL_ERROR) {
		std::cerr << "Failed to send message size: " << strerror(errno) << std::endl;
	}
	if (::send(socket, bytes.data(), sent_bytes, 0) == SYSCALL_ERROR) {
		std::cerr << "Failed to send message data: " << strerror(errno) << std::endl;
	}
}

void Server::sendTo(const std::string& message, long long client_id) {
	auto it = _clients.find(client_id);
	if (it != _clients.end()) {
		std::cout << "Sending message to client ID " << client_id << std::endl;
		reply(it->second ,message);
		flush(it->second);
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
	for (const auto& client : _clients) {
		sendToSocket(client.second.fd, message);
	}
}

void Server::updatePoll() {
	for (auto it = _clients.begin(); it != _clients.end();) {
		if (it->second.closing) {
			close(it->second.fd);
			std::cout << "Client socket closed for client: " << it->first << std::endl;
			it = _clients.erase(it);
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

	for (const auto& client : _clients) {
		pollfd p{};
		p.fd = client.second.fd;
		p.events = POLLIN | (client.second.out.empty() ? 0 : POLLOUT);
		fds.push_back(p);
		ids.push_back(client.first);
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
				long long client_id = _next_client_id++;
				Client& client = _clients.emplace(client_id, Client(client_socket, client_id)).first->second;
				std::cout << "New client connected with ID: " << client_id << std::endl;
				reply(client, SERVER_GREETING);
				flush(client);
			}
		}
	}

	for (size_t i = POLL_CLIENT_START; i < fds.size(); i++) {
		auto it = _clients.find(ids[i]);
		if (it == _clients.end())
			continue;
		Client& client = it->second;
		if (fds[i].revents & (POLLIN | POLLHUP | POLLERR))
			readFrom(client);
		flush(client);
	}
}

void Server::readFrom(Client& c) {
	char buff[RECV_BUFFER_SIZE];

	while (true) {
		ssize_t n = ::recv(c.fd, buff, sizeof(buff), 0);
		if (n > 0) {
			c.in.append(buff, n);
			continue;
		}
		if (n == 0) {
			c.closing = true;
			break;
		}
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			break;
		if (errno == EINTR)
			continue;

		c.closing = true;
		break;
	}
	size_t pos;
	while ((pos = c.in.find(LINE_END)) != std::string::npos) {
		std::string line = c.in.substr(0, pos);
		c.in.erase(0, pos + 1);
		if (!line.empty() && line.back() == CARRIAGE_RETURN)
			line.pop_back();
		handeLine(c, line);
	}
	// if c.in.size() trop long, a faire
}

void Server::handeLine(Client& c, const std::string& line)
{
	size_t sp = line.find(CMD_SEPARATOR);
	std::string cmd = line.substr(0, sp);
	std::string args = (sp == std::string::npos) ? "" : line.substr(sp + 1);

	auto it = _actions.find(cmd);
	if (it != _actions.end())
	{
		it->second(c.id, args);}
}

void Server::reply(Client& c, const std::string& line) {
	c.out += line; c.out += LINE_END;
}

void Server::flush(Client& c) {
    while (!c.out.empty()) {
        ssize_t n = ::send(c.fd, c.out.data(), c.out.size(), MSG_NOSIGNAL);
        if (n > 0) {
			c.out.erase(0, n);
			continue;
		}
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			break;
        c.closing = true; break;
    }
}

bool Server::getStatus() const {
	return _running;
}