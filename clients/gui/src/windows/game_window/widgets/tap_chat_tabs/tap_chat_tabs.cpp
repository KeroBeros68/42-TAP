#include "tap_chat_tabs.hpp"

TAPChatTabs::TAPChatTabs()
{
    this->_section_name.setText("CHATROOMS");
    this->_layout.addWidget(&this->_section_name, 0, Qt::AlignCenter);
    this->_layout.addWidget(&this->_tabbar_container, 1);

    // Fill tabs
    this->_tabbar_container.addTab(&this->_global_chat_widget, "Global");
    this->_tabbar_container.addTab(&this->_room_chat_widget, "Room");
    this->_tabbar_container.addTab(&this->_group_chat_widget, "Group");
    
    this->setLayout(&this->_layout);
}

void TAPChatTabs::add_global_message(std::map<std::string, std::string> data)
{
    this->_global_chat_widget.add_message(data);
}

void TAPChatTabs::add_room_message(std::map<std::string, std::string> data)
{
    this->_room_chat_widget.add_message(data);
}

void TAPChatTabs::add_group_message(std::map<std::string, std::string> data)
{
    this->_group_chat_widget.add_message(data);
}

void TAPChatTabs::clear_messages_history()
{
    this->_global_chat_widget.clear_messages_history();
    this->_room_chat_widget.clear_messages_history();
    this->_group_chat_widget.clear_messages_history();
}