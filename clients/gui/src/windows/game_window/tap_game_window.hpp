#ifndef TAP_GAME_WINDOW_HPP
# define TAP_GAME_WINDOW_HPP

# include "../windows.hpp"
# include "../../custom_ui_elements/custom_ui_elements.hpp"
# include "widgets/game_window_widgets.hpp"

class TAPGameWindow : public TAPBaseWindow
{
    private:
        // GUI
        TAPPlayerCountBar player_count_bar;
        TAPRoomView room_view;
        QVBoxLayout _window_layout;

    public:
        // Constructor
        TAPGameWindow();
        // ~TAPGameWindow();

        // Update labels
        void update_players_in_room_label(int value);
        void update_total_players_label(int value);
        void update_room_name(std::string name);
        void update_room_description(std::string description);

};

#endif // !TAP_GAME_WINDOW_HPP
