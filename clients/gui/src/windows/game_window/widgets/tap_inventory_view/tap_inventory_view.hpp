#ifndef TAP_INVENTORY_VIEW_HPP
# define TAP_INVENTORY_VIEW_HPP

# include "globals.hpp"
# include "src/custom_ui_elements/custom_ui_elements.hpp"
# include "../listed_item.hpp"
# include "../../../../../../state/session.hpp"

class TAPInventoryView : public TAPGameSectionWidget
{
    private:
        QVBoxLayout _layout;
        QVBoxLayout _layout_items;
        QHBoxLayout _layout_actions;    // DROP and USE side by side
        TAPRoomNameLabel _section_title;
        TAPButton _validate_button;
        TAPButton _use_button;
        TAPLabel    _no_items_label;
        // items list : one entry per item, even when two items share an id
        std::vector<ListedItem> _items;
        std::vector<QRadioButton *> _item_buttons;
        Session     &_session;

    public:
        explicit TAPInventoryView(Session &session);
        void update_inventory(std::vector<ListedItem> data);
        std::string get_selected_item_id(void) const;
};

#endif // !TAP_INVENTORY_VIEW_HPP