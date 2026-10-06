#include "tap_player_count_bar.hpp"
#include "../../../../../globals.hpp"

TAPPlayerCountBar::TAPPlayerCountBar()
{
    // Init player count labels
    this->_label_players_in_room.setText((std::string(DEFAULT_PLAYER_IN_ROOM_MESSAGE) + std::string("0")).c_str());
    this->_label_total_players.setText((std::string(DEFAULT_TOTAL_PLAYERS_MESSAGE) + std::string("0")).c_str());

    // Organize layout
    this->_layout.addWidget(&this->_label_players_in_room, 0, Qt::AlignCenter);
    this->_layout.addStretch();
    this->_layout.addWidget(&this->_label_total_players, 0, Qt::AlignCenter);
    this->setLayout(&this->_layout);
};

void TAPPlayerCountBar::update_players_in_room_label(int value)
{
    this->_label_players_in_room.setText(
        (std::string(DEFAULT_PLAYER_IN_ROOM_MESSAGE) + std::to_string(value)).c_str()
    );
}

void TAPPlayerCountBar::update_total_players_label(int value)
{
    this->_label_total_players.setText(
        (std::string(DEFAULT_TOTAL_PLAYERS_MESSAGE) + std::to_string(value)).c_str()
    );
}
