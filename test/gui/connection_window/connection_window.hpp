#ifndef CONNECTION_WINDOW_HPP
# define CONNECTION_WINDOW_HPP

# include <QtWidgets>

class TAPConnectionWindow : public QWidget
{
	private:
		std::string	_remote_address; // Server address

	public:
		void		setRemoteAddress();
		std::string	getRemoteAddress();
};

#endif // !CONNECTION_WINDOW_HPP
