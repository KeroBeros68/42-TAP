#include "tap_game_window.hpp"
#include "globals.hpp"
#include <QWidget>
#include <QScreen>
#include <QGuiApplication>
#include <algorithm>

// constructor
TAPGameWindow::TAPGameWindow(Client &client, TAPErrorWindow  &error_window)
    : _client(client), _error_window(error_window), action_buttons(client)
{
    // Set window properties, clamped to the screen's available geometry
    //  (the menu bar and the dock are excluded from availableGeometry)
    QRect available = QGuiApplication::primaryScreen()->availableGeometry();
    this->resize(std::min(WINDOW_WIDTH, available.width()),
        std::min(WINDOW_HEIGHT, available.height()));
    this->setWindowTitle(GAME_WINDOW_TITLE);
    this->setLayout(&this->_window_layout);

    // Add widgets to the window layout
    //  The player count bar spans the full window width,
    //  everything except the action buttons goes in the content column,
    //  the action buttons column spans the remaining height on the right
    this->_window_layout.addWidget(&player_count_bar);

    this->_content_layout.addWidget(&room_view);
    this->_content_layout.addWidget(&exit_view);
    // Ground items and inventory side by side
    this->_items_row_layout.addWidget(&ground_items_view);
    this->_items_row_layout.addWidget(&inventory_view);
    this->_content_layout.addLayout(&this->_items_row_layout);
    this->_content_layout.addWidget(&this->npc_talk_view);
    this->_content_layout.addWidget(&this->quests_view);
    this->_content_layout.addWidget(&this->chat_tabs);
    this->_content_layout.addStretch();

    this->_columns_layout.addLayout(&this->_content_layout, 1);
    this->_columns_layout.addLayout(&this->_actions_side_layout, 1);
    this->_window_layout.addLayout(&this->_columns_layout, 1);

    this->_actions_side_layout.addWidget(&this->action_buttons);
    this->_actions_side_layout.addWidget(&this->log_view, 1);

    // std::cout is redirected through the log view,
    //  so every client print (WIP actions, etc.) shows in the logs
    this->log_view.install_as_cout();
};

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
};

void TAPGameWindow::update_inventory(std::map<std::string, std::string> data)
{
    this->inventory_view.update_inventory(data);
};

void TAPGameWindow::update_npcs(std::map<std::string, std::string> data)
{
    this->npc_talk_view.update_npcs(data);
};

void TAPGameWindow::update_quests(std::vector<std::map<std::string, std::string>> data)
{
    this->quests_view.update_quests(data);
};

void TAPGameWindow::add_global_message(std::map<std::string, std::string> data)
{
    this->chat_tabs.add_global_message(data);
};

void TAPGameWindow::add_room_message(std::map<std::string, std::string> data)
{
    this->chat_tabs.add_room_message(data);
};

void TAPGameWindow::add_group_message(std::map<std::string, std::string> data)
{
    this->chat_tabs.add_group_message(data);
};

void TAPGameWindow::clear_messages_history()
{
    this->chat_tabs.clear_messages_history();
};
