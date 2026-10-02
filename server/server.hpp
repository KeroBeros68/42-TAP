#ifndef SERVER_HPP
# define SERVER_HPP

# include <unordered_map>
# include <functional>
# include <iostream>
# include <vector>
# include <algorithm>
# include <poll.h>
# include <cstring>
# include <cerrno>
# include <sys/socket.h>
# include <netinet/in.h>
# include <fcntl.h>
# include <unistd.h>

# include "defines.hpp"
# include "errors.hpp"
# include "signalFd.hpp"

class Server {
	private:
		struct Client {
			int	fd;
			long long id;
			std::string ip;
			std::string in;
			std::string out;
			bool		authenticated = false;
			std::string	name;
			bool		closing = false;

			Client(int fd, long long id);
		};
		int _listen_socket = INVALID_FD;
		std::unordered_map<long long, Client> _clients;
		long long _next_client_id = 0;
		std::unordered_map<std::string, std::function<void(long long&, const std::string&)>> _actions;
		void sendToSocket(int socket, const std::string& message);
		bool _running = true;
		SignalFd _sigs;
		std::vector<long long> _available_id;

	public:
		Server();
		~Server();

		void start(const size_t& port);
		void defineAction(const std::string& type, const std::function<void(long long&, const std::string&)>& action);
		void sendTo(const std::string& message, long long client_id);
		void sendToArray(const std::string& message, std::vector<long long> clientIDs);
		void sendToAll(const std::string& message);
		void updatePoll();
		void readFrom(Client& c);
		void handeLine(Client& c, const std::string& line);
		void reply(Client& c, const std::string& line);
		void flush(Client& c);
		bool getStatus() const;
};

#endif