#include "tap_game_window.hpp"
#include "../../../globals.hpp"
#include <QWidget>

// constructor
TAPGameWindow::TAPGameWindow()
{
    // Set window properties
	this->resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    this->setWindowTitle(GAME_WINDOW_TITLE);
    QVBoxLayout *window_layout = new QVBoxLayout();
    this->setLayout(window_layout);

    // Add widgets to the window layout
    window_layout->addWidget(player_count_bar);
    window_layout->addStretch();
};

void TAPGameWindow::update_players_in_room_label(int value)
{
    this->player_count_bar->update_players_in_room_label(value);
};

void TAPGameWindow::update_total_players_label(int value)
{
    this->player_count_bar->update_total_players_label(value);
};
