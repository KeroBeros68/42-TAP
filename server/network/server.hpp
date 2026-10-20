#ifndef SERVER_HPP
# define SERVER_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <algorithm>
# include <arpa/inet.h>
# include <cerrno>
# include <cstring>
# include <fcntl.h>
# include <functional>
# include <iostream>
# include <netinet/in.h>
# include <poll.h>
# include <sys/socket.h>
# include <unistd.h>
# include <unordered_map>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../shared/protocol/errors.hpp"
# include "../../shared/logging/logger.hpp"
# include "../../shared/protocol/response.hpp"
# include "../../shared/system/signalFd.hpp"
# include "../../shared/protocol/success.hpp"
# include "../defines.hpp"
# include "connection.hpp"
# include "user.hpp"

//##############################################################################
//#                                   SERVER                                   #
//##############################################################################

class Server {
	private:
		//########################### TYPES ############################
		struct Session {
			Connection	conn;
			User		user;

			/**
			 * Builds the session of a new client.
			 *
			 * @param fd the socket of the client
			 * @param ip address of the client
			 * @param id id of the client
			 */
			Session(int fd, const std::string& ip, long long id);
		};

		//############################ LOOP ############################
		int _listen_socket = INVALID_FD;
		bool _running = true;
		SignalFd _sigs;

		//########################## CLIENTS ###########################
		std::unordered_map<long long, Session> _sessions;
		long long _next_client_id = 0;
		std::vector<long long> _available_id;

		//########################## COMMANDS ##########################
		std::unordered_map<std::string, std::function<Response(User&, const std::string&)>> _actions;
		std::function<void(User&)> _onDisconnect;

		//############################# IO #############################
		/**
		 * Reads what a client sent and handles each complete line.
		 * A client that closes the connection, or that sends a line that is too long, is marked to be closed.
		 *
		 * @param s the session of the client
		 */
		void readFrom(Session& s);

		/**
		 * Handles one line of a client: finds its command, runs it and sends the response.
		 * Before CONNECT, any other command is refused. A command that is not defined is an error.
		 *
		 * @param s    the session of the client
		 * @param line the line, without its line end
		 */
		void handleLine(Session& s, const std::string& line);

		/**
		 * Runs the action of a command. An exception is logged and becomes an INTERNAL_ERROR response.
		 *
		 * @param action the action of the command
		 * @param s      the session of the client
		 * @param args   the arguments of the command
		 * @return       the response of the action, or INTERNAL_ERROR if it threw
		 */
		Response runAction(const std::function<Response(User&, const std::string&)>& action, Session& s, const std::string& args);

		/**
		 * Queues a line for a client.
		 *
		 * @param c    the connection of the client
		 * @param line the line, without its line end
		 */
		void reply(Connection& c, const std::string& line);

		/**
		 * Queues the line of an error for a client.
		 *
		 * @param c     the connection of the client
		 * @param error the error
		 */
		void replyError(Connection& c, TapError error);

		/**
		 * Sends what is waiting for a client. If it cannot be sent, the connection is marked to be closed.
		 *
		 * @param c the connection of the client
		 */
		void flush(Connection& c);

	public:
		//######################## CONSTRUCTORS ########################
		/** Builds a server that does not listen yet. */
		Server();

		/** Closes the sockets of the server and of its clients. */
		~Server();

		//######################### LIFECYCLE ##########################
		/**
		 * Starts to listen, and to watch the signals that stop the server.
		 * Throws std::runtime_error if a step fails.
		 *
		 * @param port the TCP port
		 */
		void start(const size_t& port);

		/** Runs one turn of the loop: closes the clients that left, waits for something to happen (at most POLL_TIMEOUT_MS), then accepts, reads and sends. */
		void updatePoll();

		/** Tells whether the server runs (it stops on SIGINT, SIGTERM or SIGQUIT). */
		bool getStatus() const;

		//########################## COMMANDS ##########################
		/**
		 * Defines what a command does.
		 *
		 * @param type   the command, like CMD_LOOK
		 * @param action called as action(user, arguments), gives the response
		 */
		void defineAction(const std::string& type, const std::function<Response(User&, const std::string&)>& action);

		/**
		 * Defines what happens when a client leaves.
		 *
		 * @param callback called with the user, before his session is removed
		 */
		void setOnDisconnect(const std::function<void(User&)>& callback);

		//######################### MESSAGING ##########################
		/**
		 * Queues a line for a client; flushAll() sends it. A warning is logged if there is no such client.
		 *
		 * @param message   the line, without its line end
		 * @param client_id id of the client
		 */
		void sendTo(const std::string& message, long long client_id);

		/**
		 * Queues a line for some clients; flushAll() sends it.
		 *
		 * @param message   the line, without its line end
		 * @param clientIDs ids of the clients
		 */
		void sendToArray(const std::string& message, const std::vector<long long>& clientIDs);

		/**
		 * Queues a line for every client; flushAll() sends it.
		 *
		 * @param message the line, without its line end
		 */
		void sendToAll(const std::string& message);

		/** Sends what is queued for every client, in one send per client. What cannot be sent yet waits for the next turn. */
		void flushAll();

		/**
		 * Marks a client to be closed at the next turn of the loop.
		 *
		 * @param client_id id of the client
		 */
		void disconnect(long long client_id);
};

#endif
