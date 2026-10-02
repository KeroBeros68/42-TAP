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
# include "../shared/errors.hpp"
# include "signalFd.hpp"
# include "../shared/success.hpp"
# include "../shared/response.hpp"
# include "connection.hpp"
# include "user.hpp"

class Server {
	private:
		struct Session {
			Connection	conn;
			User		user;

			Session(int fd, long long id);
		};

		int _listen_socket = INVALID_FD;
		std::unordered_map<long long, Session> _sessions;
		long long _next_client_id = 0;
		std::unordered_map<std::string, std::function<Response(User&, const std::string&)>> _actions;
		bool _running = true;
		SignalFd _sigs;
		std::vector<long long> _available_id;

	public:
		Server();
		~Server();

		void start(const size_t& port);
		void defineAction(const std::string& type, const std::function<Response(User&, const std::string&)>& action);
		void sendTo(const std::string& message, long long client_id);
		void sendToArray(const std::string& message, std::vector<long long> clientIDs);
		void sendToAll(const std::string& message);
		void updatePoll();
		void readFrom(Session& s);
		void handeLine(Session& s, const std::string& line);
		void reply(Connection& c, const std::string& line);
		void flush(Connection& c);
		bool getStatus() const;
};

#endif