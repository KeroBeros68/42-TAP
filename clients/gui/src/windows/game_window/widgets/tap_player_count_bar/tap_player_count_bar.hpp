#ifndef TAP_PLAYER_COUNT_BAR_HPP
# define TAP_PLAYER_COUNT_BAR_HPP

# include "../../../../custom_ui_elements/custom_ui_elements.hpp"
# include <QWidget>

class TAPPlayerCountBar : public TAPGameSectionWidget
{
    private:
        QHBoxLayout *_layout = new QHBoxLayout();
        TAPLabel    _label_players_in_room;
        TAPLabel    _label_total_players;

    public:
        TAPPlayerCountBar();

        // Update number of players in room
        void update_players_in_room_label(int value);
        void update_total_players_label(int value);
};

#endif // !TAP_PLAYER_COUNT_BAR_HPP
