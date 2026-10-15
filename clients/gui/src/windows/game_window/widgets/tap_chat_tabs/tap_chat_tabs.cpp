#include "tap_chat_tabs.hpp"

TAPChatTabs::TAPChatTabs()
{
    this->_section_name.setText("CHATROOMS");
    this->_layout.addWidget(&this->_section_name, 0, Qt::AlignCenter);
    this->_layout.addWidget(&this->_tabbar_container, 0, Qt::AlignCenter);

    // Fill tabs
    this->_tabbar_container.addTab("Global");
    this->_tabbar_container.addTab("Room");
    this->_tabbar_container.addTab("Group");
    
    this->setLayout(&this->_layout);
}