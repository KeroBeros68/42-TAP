#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <iostream>
# include <string>
# include <stdexcept>
# include <vector>
# include <fcntl.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <poll.h>
# include <cerrno>
# include <cstring>
# include <functional>
# include <unordered_map>

# include "defines.hpp"

class Client
{
	private:
		int _fd;
		std::string _servername;
		int _port;
		bool _is_authenticated = false;
		std::string _in;
		std::string _kbd;
		bool _keyboard = false;
		std::unordered_map<std::string, std::function<void(const std::string&)>> _actions;
		void handleLine(const std::string& line);
		bool readServer();
		bool readKeyboard();

	public:
		Client();
		~Client();

		void connect();
		void disconnect();
		bool isConnected() const;
		bool isAuthenticated() const;
		int fd() const;

		// Handler called with the rest of the line when a server message starts with `type`
		// ("OK", "ERR", "EVT"...)
		void defineAction(const std::string& type, const std::function<void(const std::string&)>& action);
		// Send one line to the server (LINE_END is added)
		bool send(const std::string& message);
		// Also read the keyboard in update(): each typed line is sent to the server (CLI only)
		void enableKeyboard(bool enable = true);
		// Wait up to timeout_ms for server data (and keyboard input if enabled).
		// Server lines run the matching actions, typed lines are sent to the server.
		// Returns false if the connection was lost or the keyboard input ended (Ctrl+D).
		bool update(int timeout_ms = POLL_TIMEOUT_MS);
};

#endif