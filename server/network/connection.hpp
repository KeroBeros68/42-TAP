#ifndef CONNECTION_HPP
# define CONNECTION_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                 CONNECTION                                 #
//##############################################################################

class Connection {
	private:
		//######################### VARIABLES ##########################
		int			_fd;
		std::string	_ip;
		std::string	_in;
		std::string	_out;
		bool		_closing = false;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Builds the connection of a client.
		 *
		 * @param fd the socket of the client
		 * @param ip address of the client, for the logs
		 */
		Connection(int fd, const std::string& ip);

		//########################## IDENTITY ##########################
		/** Returns the socket of the client. */
		int fd() const;

		/** Returns the address of the client. */
		const std::string& ip() const;

		//########################### INPUT ############################
		/** Returns the bytes received and not used yet. */
		const std::string& in() const;

		/**
		 * Adds bytes that were received.
		 *
		 * @param data the bytes
		 * @param size how many there are
		 */
		void appendIn(const char* data, size_t size);

		/**
		 * Takes the first complete line out of the bytes received.
		 *
		 * @param line receives the line, without its line end
		 * @return     false if there is no complete line yet
		 */
		bool popLine(std::string& line);

		/** Forgets the bytes received and not used yet. */
		void clearIn();

		//########################### OUTPUT ###########################
		/** Returns the bytes that are waiting to be sent. */
		const std::string& out() const;

		/**
		 * Adds bytes to send.
		 *
		 * @param data the bytes
		 */
		void appendOut(const std::string& data);

		/**
		 * Removes the bytes that were sent.
		 *
		 * @param count how many were sent
		 */
		void consumeOut(size_t count);

		//########################### STATE ############################
		/** Marks the connection as one to close (or not) at the next turn of the loop. */
		void setClosing(bool closing);

		/** Tells whether the connection has to be closed. */
		bool isClosing() const;
};

#endif
