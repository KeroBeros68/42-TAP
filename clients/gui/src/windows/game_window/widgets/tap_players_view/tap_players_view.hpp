#ifndef TAP_PLAYERS_VIEW_HPP
# define TAP_PLAYERS_VIEW_HPP

# include "globals.hpp"
# include "src/custom_ui_elements/custom_ui_elements.hpp"

// Widget displaying the names of the players standing in the current room.
// Read-only: the names come from the "players" array of LOOK, then from the
// EVT ROOM PRESENCE ENTER/LEAVE, and nobody is sent anything from here.
class TAPPlayersView : public TAPGameSectionWidget
{
    private:
        QVBoxLayout _layout;
        QVBoxLayout _layout_players;
        TAPRoomNameLabel _section_title;
        TAPLabel    _no_players_label;
        // players list: the names, exactly as the server sent them
        std::vector<std::string> _players;
        std::vector<TAPLabel *> _player_labels;

    public:
        TAPPlayersView();
        void update_players(std::vector<std::string> data);
};

#endif // !TAP_PLAYERS_VIEW_HPP
