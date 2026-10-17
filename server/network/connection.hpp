#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <string>

class Connection {
	private:
		int			_fd;
		std::string	_ip;
		std::string	_in;
		std::string	_out;
		bool		_closing = false;

	public:
		Connection(int fd, const std::string& ip);

		int fd() const;
		const std::string& ip() const;

		const std::string& in() const;
		void appendIn(const char* data, size_t size);
		bool popLine(std::string& line);
		void clearIn();

		const std::string& out() const;
		void appendOut(const std::string& data);
		void consumeOut(size_t count);

		void setClosing(bool closing);
		bool isClosing() const;
};

#endif
