#ifndef CONNECTION_WINDOW_HPP
# define CONNECTION_WINDOW_HPP

# include "../base_window/tap_base_window.hpp"
#include "../../custom_ui_elements/custom_ui_elements.hpp"
# include <iostream>

class TAPConnectionWindow : public TAPBaseWindow
{
	private:
		std::string	_username; // Server address
		std::string	_remote_address = "127.0.0.1"; // Server address
		std::string _remote_port	= "4224"; // Server port

		// Labels
    	TAPLabel _info_text;
		TAPLineEdit _username_filed; // Create text area for IP address
		TAPLineEdit _address_filed; // Create text area for IP address
		TAPLineEdit _port_filed; // Create text area for IP address
		QPushButton _button; // Create connect button
    	QVBoxLayout _layout;

	public:
		// init window
		TAPConnectionWindow(
			std::string window_title,
			int width,
			int height
		);

		// Username related function
		std::string findSystemUsername();
		std::string getUsername();

		// Address related functions
		void		setRemoteAddress(std::string address);
		std::string	getRemoteAddress();

		// Port related functions
		void		setRemotePort(std::string port);
		std::string	getRemotePort();

		// Connection related functions
		void		connectToServer();
};

#endif // !CONNECTION_WINDOW_HPP
