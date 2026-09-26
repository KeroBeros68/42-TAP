#ifndef CONNECTION_WINDOW_HPP
# define CONNECTION_WINDOW_HPP

# include <QtWidgets>
# include <iostream>

class TAPConnectionWindow : public QWidget
{
	private:
		std::string	_remote_address = "127.0.0.1"; // Server address
		std::string _remote_port	= "4224"; // Server port

	public:
		// init window
		TAPConnectionWindow(
			std::string window_title,
			int width,
			int height
		);

		// Address related functions
		void		setRemoteAddress(std::string address);
		std::string	getRemoteAddress();

		// Port related functions
		void		setRemotePort(std::string port);
		std::string	getRemotePort();
};

#endif // !CONNECTION_WINDOW_HPP
