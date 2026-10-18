#ifndef TAP_NPC_TALK_HPP
# define TAP_NPC_TALK_HPP

# include "globals.hpp"
# include "src/custom_ui_elements/custom_ui_elements.hpp"

class TAPNPCTalkView : public TAPGameSectionWidget
{
    private:
        QVBoxLayout _layout;
        QVBoxLayout _layout_items;
        TAPRoomNameLabel _section_title;
        TAPButton _validate_button;
        TAPButton _attack_button;
        TAPLabel    _no_items_label;
        // items list : canonical_id, human_readable_name
        std::map<std::string, std::string> _npcs;
        std::vector<QRadioButton *> _npc_buttons;

    public:
        TAPNPCTalkView();
        void update_npcs(std::map<std::string, std::string> data);
        std::string get_selected_npc_id(void) const;
};

#endif // !TAP_NPC_TALK_HPP
