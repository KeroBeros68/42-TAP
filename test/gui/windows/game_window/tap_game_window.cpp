#include "tap_game_window.hpp"
#include "../../../globals.hpp"
#include <QWidget>

// update_players_in_room_label
void TAPGameWindow::update_players_in_room_label(int value)
{
    this->_label_players_in_room->setText(
        (std::string(DEFAULT_PLAYER_IN_ROOM_MESSAGE) + std::to_string(value)).c_str()
    );

}

// update_total_players_label
void TAPGameWindow::update_total_players_label(int value)
{
    this->_label_total_players->setText(
        (std::string(DEFAULT_TOTAL_PLAYERS_MESSAGE) + std::to_string(value)).c_str()
    );
}

// constructor
TAPGameWindow::TAPGameWindow()
{
    // Set window properties
	this->resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    this->setWindowTitle(GAME_WINDOW_TITLE);
    QVBoxLayout *window_layout = new QVBoxLayout();
    this->setLayout(window_layout);

    // Init player count labels
    this->_label_players_in_room->setText((std::string(DEFAULT_PLAYER_IN_ROOM_MESSAGE) + std::string("0")).c_str());
    this->_label_total_players->setText((std::string(DEFAULT_TOTAL_PLAYERS_MESSAGE) + std::string("0")).c_str());

    // Top bar widget
    QWidget *top_bar_widget = new QWidget();
    QHBoxLayout *top_bar_layout = new QHBoxLayout;
    top_bar_layout->addWidget(this->_label_players_in_room, 0, Qt::AlignCenter);
    top_bar_layout->addStretch();
    top_bar_layout->addWidget(this->_label_total_players, 0, Qt::AlignCenter);
    top_bar_widget->setLayout(top_bar_layout);
    window_layout->addWidget(top_bar_widget);
};



// int main()                                  {
//     printf("Hello world !")                 ;
//     int value = 1                           ;

//     if (value = 1)                          {
//         printf("Good value\n")              ;
//         printf("pass")                      ;}
//     else
//         printf("Bad value :(\n")            ;}