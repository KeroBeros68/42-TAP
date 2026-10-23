#ifndef TAP_GAME_WINDOW_HPP
# define TAP_GAME_WINDOW_HPP

# include "../base_window/tap_base_window.hpp"
# include "../../custom_ui_elements/custom_ui_elements.hpp"
# include "widgets/game_window_widgets.hpp"
# include <QHBoxLayout>
# include "../error_window/tap_error_window.hpp"
#include "../../../../client.hpp"

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
        Client      	    &_client;

    public:
        // Constructor
        TAPGameWindow(
            Client          &client,
            TAPErrorWindow  &error_window
        );

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
