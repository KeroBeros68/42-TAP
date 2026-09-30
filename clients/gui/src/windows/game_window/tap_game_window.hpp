#ifndef TAP_GAME_WINDOW_HPP
# define TAP_GAME_WINDOW_HPP

# include "../windows.hpp"
# include "../../custom_ui_elements/custom_ui_elements.hpp"
# include "widgets/game_window_widgets.hpp"

class TAPGameWindow : public TAPBaseWindow
{
    private:
        // GUI
        TAPPlayerCountBar *player_count_bar = new TAPPlayerCountBar();

    public:
        // Constructor
        TAPGameWindow();

        // Update labels
        void update_players_in_room_label(int value);
        void update_total_players_label(int value);

};

#endif // !TAP_GAME_WINDOW_HPP