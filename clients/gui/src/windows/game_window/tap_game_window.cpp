#include "tap_game_window.hpp"
#include "globals.hpp"
#include <QWidget>
#include <QScreen>
#include <QGuiApplication>
#include <algorithm>

// constructor
TAPGameWindow::TAPGameWindow(Session &session, TAPErrorWindow  &error_window)
    : exit_view(session), ground_items_view(session), inventory_view(session),
    npc_talk_view(session), action_buttons(session), group_view(session),
    chat_tabs(session),
    _error_window(error_window), _session(session)
{
    // Set window properties, clamped to the screen's available geometry
    //  (the menu bar and the dock are excluded from availableGeometry)
    QRect available = QGuiApplication::primaryScreen()->availableGeometry();
    this->resize(std::min(WINDOW_WIDTH, available.width()),
        std::min(WINDOW_HEIGHT, available.height()));
    this->setWindowTitle(GAME_WINDOW_TITLE);
    this->setLayout(&this->_window_layout);

    // Add widgets to the window layout
    //  The player count bar and the player info bar span the full window width,
    //  everything except the action buttons goes in the content column,
    //  the action buttons column spans the remaining height on the right
    this->_window_layout.addWidget(&player_count_bar);
    this->_window_layout.addWidget(&player_info_bar);

    this->_content_layout.addWidget(&room_view);
    this->_content_layout.addWidget(&players_view);
    // Ground items and inventory side by side
    this->_items_row_layout.addWidget(&ground_items_view);
    this->_items_row_layout.addWidget(&inventory_view);
    this->_content_layout.addLayout(&this->_items_row_layout);
    this->_content_layout.addWidget(&this->quests_view);
    this->_content_layout.addWidget(&this->group_view);
    this->_content_layout.addWidget(&this->chat_tabs);
    this->_content_layout.addStretch();

    this->_columns_layout.addLayout(&this->_content_layout, 1);
    this->_columns_layout.addLayout(&this->_actions_side_layout, 1);
    this->_window_layout.addLayout(&this->_columns_layout, 1);

    this->_actions_side_layout.addWidget(&this->action_buttons);
    this->_actions_side_layout.addWidget(&exit_view);
    // The talk section (npcs, buttons and dialogue) and the logs share the
    // room left in the column: same stretch, so neither squeezes the other.
    this->_actions_side_layout.addWidget(&this->npc_talk_view, 1);
    this->_actions_side_layout.addWidget(&this->log_view, 1);

    // std::cout is redirected through the log view,
    //  so every client print (WIP actions, etc.) shows in the logs
    this->log_view.install_as_cout();
};

// ============================================================
// State -> widgets
// ============================================================

// Turn a list of ids into {id, display name}, one entry per id received.
// The id stays the key: it is what the server expects back in TAKE/DROP/...
// The name is only what the player reads.
// It is a vector and not a map: the server sends the definition id, so two
// herbs both arrive as "item.herbs" and a map would merge them into one entry.
std::vector<ListedItem> TAPGameWindow::named(const std::vector<std::string> &ids) const
{
    std::vector<ListedItem> result;
    result.reserve(ids.size());
    for (const std::string &id : ids)
        result.push_back({id, this->_session.catalog().name(id)});
    return result;
}

// Turn ChatLine{from, text} into the maps the chat widgets expect.
std::vector<std::map<std::string, std::string>> TAPGameWindow::chat_lines(const std::vector<ChatLine> &lines) const
{
    std::vector<std::map<std::string, std::string>> result;
    for (const ChatLine &line : lines)
        result.push_back({{"author", line.from}, {"content", line.text}});
    return result;
}

void TAPGameWindow::update_gui(Section section)
{
    switch (section)
    {
        case Section::Connection:   // handled by TAPConnectionWindow
            break;
        case Section::Room:         this->update_room_section(); break;
        case Section::Inventory:    this->update_inventory_section(); break;
        case Section::Status:       this->update_status_section(); break;
        case Section::Combat:       this->update_status_section(); break; // same labels
        case Section::Counters:     this->update_counters_section(); break;
        case Section::Chat:         this->update_chat_section(); break;
        case Section::Group:        this->update_group_section(); break;
        case Section::Quests:       this->update_quests_section(); break;
        case Section::Dialogue:     this->update_dialogue_section(); break;
        case Section::Log:          this->update_log_section(); break;
    }
}

// Name, description, exits, items on the floor, npcs in the room
void TAPGameWindow::update_room_section()
{
    const GameState &state = this->_session.state();

    this->room_view.update_room_name(state.room.name);
    this->room_view.update_room_description(state.room.description);

    // The exit view shows the destination names, not their ids
    std::map<std::string, std::string> exits;
    for (const auto &[direction, id] : state.room.exits)
        exits[direction] = this->_session.catalog().name(id);
    this->exit_view.update_available_exits(exits);

    this->ground_items_view.update_ground_items(this->named(state.items));
    this->npc_talk_view.update_npcs(this->named(state.npcs));

    // Player names come straight from the server: nothing to translate
    this->players_view.update_players(state.players);
}

void TAPGameWindow::update_inventory_section()
{
    this->inventory_view.update_inventory(this->named(this->_session.state().inventory));
}

// Our own hit points, status and name. STATUS is the only source of the hit
// points; a combat answer asks for STATUS again, so both paths end up here.
void TAPGameWindow::update_status_section()
{
    this->update_player_info_bar();
    this->update_counters_section();
}

void TAPGameWindow::update_player_info_bar()
{
    const GameState &state = this->_session.state();
    this->player_info_bar.update_player_info(state.me, state.hp, state.max_hp,
        state.status);
}

void TAPGameWindow::update_counters_section()
{
    const GameState &state = this->_session.state();
    this->update_players_in_room_label(static_cast<int>(state.players.size()));
    this->update_total_players_label(state.server_players);
}

// Quests, active and completed: the widget shows the progress the server sent
void TAPGameWindow::update_quests_section()
{
    std::vector<std::map<std::string, std::string>> quests;
    for (const GameState::Quest &quest : this->_session.state().quests)
    {
        quests.push_back({
            {"quest_id", quest.name.empty() ? quest.id : quest.name},
            {"status", quest.status},
            {"progress", quest.progress}
        });
    }
    this->quests_view.update_quests(quests);
}

// The group we are in, and the invitation we received, both read from the state
void TAPGameWindow::update_group_section()
{
    const GameState &state = this->_session.state();
    this->group_view.update_group(state.group.has_value(), state.group.value_or(""),
        state.pending_invite.value_or(""));
}

void TAPGameWindow::update_chat_section(){
    const GameState &state = this->_session.state();
    this->chat_tabs.update_global_messages(this->chat_lines(state.chat_global));
    this->chat_tabs.update_room_messages(this->chat_lines(state.chat_room));
    this->chat_tabs.update_group_messages(this->chat_lines(state.chat_group));
}

void TAPGameWindow::update_dialogue_section()
{
    this->npc_talk_view.update_dialogue(this->_session.state().dialogue);
}

void TAPGameWindow::update_log_section()
{
    this->log_view.update_logs(this->_session.state().log);
}

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

void TAPGameWindow::update_ground_items(std::vector<ListedItem> data)
{
    this->ground_items_view.update_ground_items(data);
};

void TAPGameWindow::update_inventory(std::vector<ListedItem> data)
{
    this->inventory_view.update_inventory(data);
};

void TAPGameWindow::update_npcs(std::vector<ListedItem> data)
{
    this->npc_talk_view.update_npcs(data);
};

void TAPGameWindow::update_quests(std::vector<std::map<std::string, std::string>> data)
{
    this->quests_view.update_quests(data);
};

void TAPGameWindow::update_global_messages(const std::vector<std::map<std::string, std::string>> &messages)
{
    this->chat_tabs.update_global_messages(messages);
};

void TAPGameWindow::update_room_messages(const std::vector<std::map<std::string, std::string>> &messages)
{
    this->chat_tabs.update_room_messages(messages);
};

void TAPGameWindow::update_group_messages(const std::vector<std::map<std::string, std::string>> &messages)
{
    this->chat_tabs.update_group_messages(messages);
};

void TAPGameWindow::clear_messages_history()
{
    this->chat_tabs.clear_messages_history();
};
