#include "tap_chat_widget.hpp"
#include <iostream>

TAPChatWidget::TAPChatWidget()
{
    // Connect button
	this->_send_button.setText("Send");
	this->connect(&this->_send_button,
        &QPushButton::clicked,
        this,
        [this] {
			if (_textarea.text().size() == 0) {return ;}
            std::cout << "[WIP] Sending message..." << std::endl;
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

void TAPChatWidget::add_message(std::map<std::string, std::string> message)
{
	// add the message to the list
	this->_messages.push_back(message);

	TAPLabel *label = new TAPLabel();
	label->setParent(&this->_messages_container);
	label->setText((message["author"] + std::string(" : ") + message["content"]).c_str());
	label->setWordWrap(true);
	this->_messages_labels.push_back(label);
	this->_messages_layout.insertWidget(this->_messages_layout.count() - 1, label); // before the stretch
}

void TAPChatWidget::clear_messages_history()
{
	for (auto *label : this->_messages_labels)
		delete label;
	this->_messages_labels.clear();
	this->_messages.clear();
}
