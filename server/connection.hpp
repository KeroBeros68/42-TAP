#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <string>

// Everything related to the socket of a client. Private to the server.
struct Connection {
	int			fd;
	std::string	ip;
	std::string	in;			// receive buffer
	std::string	out;		// send buffer
	bool		closing = false;

	Connection(int fd);
};

#endif
