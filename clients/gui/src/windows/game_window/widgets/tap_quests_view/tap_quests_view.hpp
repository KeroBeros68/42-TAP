#ifndef TAP_QUESTS_VIEW_HPP
# define TAP_QUESTS_VIEW_HPP

# include "globals.hpp"
# include "src/custom_ui_elements/custom_ui_elements.hpp"

class TAPQuestsView : public TAPGameSectionWidget
{
    private:
        QVBoxLayout _layout;
        QVBoxLayout _layout_items;
        TAPRoomNameLabel _section_title;
        TAPLabel    _no_quests_label;
        // quests list: each quest is a dict of quest_id, status, progress
        std::vector<std::map<std::string, std::string>> _quests;
        std::vector<TAPLabel *> _quest_labels;

    public:
        TAPQuestsView();
        void update_quests(std::vector<std::map<std::string, std::string>> data);
};

#endif // !TAP_QUESTS_VIEW_HPP
