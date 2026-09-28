#ifndef TAP_GAME_WINDOW_HPP
# define TAP_GAME_WINDOW_HPP

# include "../windows.hpp"
# include "../../custom_ui_elements/custom_ui_elements.hpp"

class TAPGameWindow : public TAPBaseWindow
{
    private:
        // GUI
        TAPLabel *_label_players_in_room = new TAPLabel();
        TAPLabel *_label_total_players = new TAPLabel();


    public:
        // Constructor
        TAPGameWindow();

        // Update labels
        void update_players_in_room_label(int value);
        void update_total_players_label(int value);

};

#endif // !TAP_GAME_WINDOW_HPP