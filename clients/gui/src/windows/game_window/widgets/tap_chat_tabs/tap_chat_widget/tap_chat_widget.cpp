#include "tap_chat_widget.hpp"

TAPChatWidget::TAPChatWidget(Session &session, const std::string &scope)
	: _session(session), _scope(scope)
{
    // Connect button
	this->_send_button.setText("Send");
	this->connect(&this->_send_button,
        &QPushButton::clicked,
        this,
        [this] {
			if (_textarea.text().size() == 0) {return ;}
			// "CHAT <scope> <message>": the server answers OK, and sends the
			// message back to us as an EVT, which is what fills the history
			this->_session.send("CHAT", this->_scope + " " + _textarea.text().toStdString());
			_textarea.setText("");
    });

	// Scrollable messages history
	this->_messages_container.setLayout(&this->_messages_layout);
	this->_messages_layout.addStretch();
	this->_messages_scroll_area.setWidget(&this->_messages_container);
	this->_messages_scroll_area.setWidgetResizable(true);

	// Send message area : line edit + button side by side
	this->_send_message_area.addWidget(&this->_textarea);
	this->_send_message_area.addWidget(&this->_send_button);

	this->_layout.addWidget(&this->_messages_scroll_area);
	this->_layout.addLayout(&this->_send_message_area);

	this->setLayout(&this->_layout);
}

TAPChatWidget::~TAPChatWidget()
{
	this->clear_messages_history();
}

void TAPChatWidget::update_messages(const std::vector<std::map<std::string, std::string>> &messages)
{
	// Drop the labels of the previous history, then rebuild them all.
	// Rebuilding is cheap for a small chat and keeps one single path:
	// the widget always shows exactly what it was given.
	this->clear_messages_history();

	for (const std::map<std::string, std::string> &message : messages)
	{
		this->_messages.push_back(message);

		TAPLabel *label = new TAPLabel();
		label->setParent(&this->_messages_container);
		label->setText((message.at("author") + std::string(" : ") + message.at("content")).c_str());
		label->setWordWrap(true);
		this->_messages_labels.push_back(label);
		this->_messages_layout.insertWidget(this->_messages_layout.count() - 1, label); // before the stretch
	}
}

void TAPChatWidget::clear_messages_history()
{
	for (auto *label : this->_messages_labels)
		delete label;
	this->_messages_labels.clear();
	this->_messages.clear();
}
