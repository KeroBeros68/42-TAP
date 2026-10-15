#ifndef TAP_CHAT_TABS_HPP
# define TAP_CHAT_TABS_HPP

#include "src/custom_ui_elements/custom_ui_elements.hpp"

class TAPChatTabs : public TAPGameSectionWidget
{
    private:
        TAPRoomNameLabel    _section_name;
        QVBoxLayout         _layout;
        QTabWidget          _tabbar_container;

    public:
        TAPChatTabs();
};

#endif // !TAP_CHAT_TABS_HPP