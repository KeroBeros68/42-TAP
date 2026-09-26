#ifndef CONNECTION_WINDOW_HPP
# define CONNECTION_WINDOW_HPP

# include <QtWidgets>
# include <iostream>

class TAPConnectionWindow : public QWidget
{
	private:
		std::string	_remote_address = "127.0.0.1"; // Server address

	public:
		TAPConnectionWindow(
			std::string window_title,
			int width,
			int height
		);
		void		setRemoteAddress(std::string address);
		std::string	getRemoteAddress();
};

#endif // !CONNECTION_WINDOW_HPP
