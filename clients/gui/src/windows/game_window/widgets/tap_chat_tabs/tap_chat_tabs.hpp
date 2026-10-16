#ifndef TAP_CHAT_TABS_HPP
# define TAP_CHAT_TABS_HPP

#include "src/custom_ui_elements/custom_ui_elements.hpp"
#include "tap_chat_widget/tap_chat_widget.hpp"

class TAPChatTabs : public TAPGameSectionWidget
{
    private:
        TAPRoomNameLabel    _section_name;
        QVBoxLayout         _layout;
        QTabWidget          _tabbar_container;
        TAPChatWidget       _global_chat_widget;
        TAPChatWidget       _room_chat_widget;
        TAPChatWidget       _group_chat_widget;

    public:
        TAPChatTabs();

        // Chat passthroughs
        void add_global_message(std::map<std::string, std::string> data);
        void add_room_message(std::map<std::string, std::string> data);
        void add_group_message(std::map<std::string, std::string> data);
        void clear_messages_history();
};

#endif // !TAP_CHAT_TABS_HPP