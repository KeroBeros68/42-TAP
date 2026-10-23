#ifndef TAP_CHAT_TABS_HPP
# define TAP_CHAT_TABS_HPP

#include "src/custom_ui_elements/custom_ui_elements.hpp"
#include "tap_chat_widget/tap_chat_widget.hpp"
#include <map>
#include <string>
#include <vector>

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

        // Replace the history of each scope by the one given.
        // Each message has an "author" and a "content" key.
        void update_global_messages(const std::vector<std::map<std::string, std::string>> &messages);
        void update_room_messages(const std::vector<std::map<std::string, std::string>> &messages);
        void update_group_messages(const std::vector<std::map<std::string, std::string>> &messages);

        void clear_messages_history();
};

#endif // !TAP_CHAT_TABS_HPP