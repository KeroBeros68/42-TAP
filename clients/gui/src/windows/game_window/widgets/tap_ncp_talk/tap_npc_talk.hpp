#ifndef TAP_NPC_TALK_HPP
# define TAP_NPC_TALK_HPP

# include "globals.hpp"
# include "src/custom_ui_elements/custom_ui_elements.hpp"
# include "../listed_item.hpp"
# include "../../../../../../state/session.hpp"

class TAPNPCTalkView : public TAPGameSectionWidget
{
    private:
        QVBoxLayout _layout;
        QVBoxLayout _layout_items;
        TAPRoomNameLabel _section_title;
        TAPButton _validate_button;
        TAPButton _quest_button;
        TAPButton _attack_button;
        QHBoxLayout _combat_buttons_layout;
        TAPButton _defend_button;
        TAPButton _flee_button;
        TAPLabel    _no_items_label;
        // npcs list : one entry per npc, even when two npcs share an id
        std::vector<ListedItem> _npcs;
        std::vector<QRadioButton *> _npc_buttons;
        Session     &_session;

    public:
        explicit TAPNPCTalkView(Session &session);
        void update_npcs(std::vector<ListedItem> data);
        std::string get_selected_npc_id(void) const;
};

#endif // !TAP_NPC_TALK_HPP
