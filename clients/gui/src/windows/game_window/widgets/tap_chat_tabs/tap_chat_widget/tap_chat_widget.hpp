#ifndef TAP_CHAT_WIDGET_HPP
# define TAP_CHAT_WIDGET_HPP

# include <vector>
# include <QWidget>

# include "src/custom_ui_elements/custom_ui_elements.hpp"
# include "../../../../../../../state/session.hpp"

class TAPChatWidget : public QWidget
{
	private:
		std::vector<std::map<std::string, std::string>> _messages; // Messages history with "author" and "content" keys
		std::vector<TAPLabel *>							_messages_labels; // QT Labels displaying the messages
		QScrollArea										_messages_scroll_area; // Scroll area when there are too much messages
		QWidget											_messages_container; // Container containing messages, in the scroll area
		QVBoxLayout										_messages_layout; // Div for vertical arrangement
		TAPLineEdit										_textarea; // for writing messages
		TAPButton										_send_button; // for sending written messages
		QVBoxLayout										_layout; // chat widget's whole layout
		QHBoxLayout										_send_message_area; // horizontal widget for the send area/button

		Session		&_session;
		std::string	_scope; // CHAT_SCOPE_GLOBAL, CHAT_SCOPE_ROOM or CHAT_SCOPE_GROUP

	public:
		TAPChatWidget(Session &session, const std::string &scope);
		~TAPChatWidget();

		// Replace the whole history by the one given, and rebuild the labels.
		// Keys of each message: "author" and "content".
		void update_messages(const std::vector<std::map<std::string, std::string>> &messages);

		void clear_messages_history();

};

#endif // !TAP_CHAT_WIDGET_HPP