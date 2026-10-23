#ifndef TAP_GAME_WINDOW_HPP
# define TAP_GAME_WINDOW_HPP

# include "../base_window/tap_base_window.hpp"
# include "../../custom_ui_elements/custom_ui_elements.hpp"
# include "widgets/game_window_widgets.hpp"
# include <QHBoxLayout>
# include "../error_window/tap_error_window.hpp"
#include "../../../../state/session.hpp"

class TAPGameWindow : public TAPBaseWindow
{
    private:
        // GUI
        QVBoxLayout         _window_layout; // Player count bar on top, then the two columns below
        QHBoxLayout         _columns_layout; // Content on the left, action buttons on the right
        QVBoxLayout         _content_layout; // Everything except the action buttons
        QHBoxLayout         _items_row_layout; // Ground items and inventory side by side
        QVBoxLayout         _actions_side_layout; // Action buttons column on the right
        TAPPlayerCountBar   player_count_bar;
        TAPRoomView         room_view;
        TAPExitView         exit_view;
        TAPGroundItemsView  ground_items_view;
        TAPInventoryView    inventory_view;
        TAPNPCTalkView      npc_talk_view;
        TAPQuestsView       quests_view;
        TAPActionButtons    action_buttons;
        TAPLogView          log_view;
        TAPChatTabs         chat_tabs;

        // Windows
        TAPErrorWindow	    &_error_window;
        Session     	    &_session;

        // One redraw per section, each reading the state it needs.
        // ids -> {id, display name}: the Catalog turns opaque ids into text.
        std::map<std::string, std::string> named(const std::vector<std::string> &ids) const;
        std::vector<std::map<std::string, std::string>> chat_lines(const std::vector<ChatLine> &lines) const;

        void update_room_section();
        void update_inventory_section();
        void update_status_section();
        void update_counters_section();
        void update_chat_section();
        void update_log_section();

    public:
        // Constructor
        TAPGameWindow(
            Session         &session,
            TAPErrorWindow  &error_window
        );

        // Read the state of the session and redraw one part of the window.
        // The window never parses a protocol line: it reads GameState only.
        void update_gui(Section section);

        // Update labels
        void update_players_in_room_label(int value);
        void update_total_players_label(int value);
        void update_room_name(std::string name);
        void update_room_description(std::string description);
        void update_available_exits(std::map<std::string, std::string> data);
        void update_ground_items(std::map<std::string, std::string> data);
        void update_inventory(std::map<std::string, std::string> data);
        void update_npcs(std::map<std::string, std::string> data);
        void update_quests(std::vector<std::map<std::string, std::string>> data);
        void update_global_messages(const std::vector<std::map<std::string, std::string>> &messages);
        void update_room_messages(const std::vector<std::map<std::string, std::string>> &messages);
        void update_group_messages(const std::vector<std::map<std::string, std::string>> &messages);
        void clear_messages_history();
};

#endif // !TAP_GAME_WINDOW_HPP
