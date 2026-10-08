#include "tap_game_window.hpp"
#include "globals.hpp"
#include <QWidget>

// constructor
TAPGameWindow::TAPGameWindow()
{
    // Set window properties
	this->resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    this->setWindowTitle(GAME_WINDOW_TITLE);
    this->setLayout(&this->_window_layout);

    // Add widgets to the window layout
    this->_window_layout.addWidget(&player_count_bar);
    this->_window_layout.addWidget(&room_view);
    this->_window_layout.addWidget(&exit_view);
    this->_window_layout.addWidget(&ground_items_view);
    this->_window_layout.addStretch();
};

// TAPGameWindow::~TAPGameWindow() {
//     if (&this->_window_layout)
//         delete &this->_window_layout;
// }

void TAPGameWindow::update_players_in_room_label(int value)
{
    this->player_count_bar.update_players_in_room_label(value);
};

void TAPGameWindow::update_total_players_label(int value)
{
    this->player_count_bar.update_total_players_label(value);
};

void TAPGameWindow::update_room_name(std::string name)
{
    this->room_view.update_room_name(name);
};

void TAPGameWindow::update_room_description(std::string description)
{
    this->room_view.update_room_description(description);
};

void TAPGameWindow::update_available_exits(std::map<std::string, std::string> data)
{
    this->exit_view.update_available_exits(data);
};

void TAPGameWindow::update_ground_items(std::map<std::string, std::string> data)
{
    this->ground_items_view.update_ground_items(data);
}
