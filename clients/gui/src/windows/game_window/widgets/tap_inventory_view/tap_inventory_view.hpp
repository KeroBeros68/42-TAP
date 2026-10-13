#ifndef TAP_INVENTORY_VIEW_HPP
# define TAP_INVENTORY_VIEW_HPP

# include "globals.hpp"
# include "src/custom_ui_elements/custom_ui_elements.hpp"

class TAPInventoryView : public TAPGameSectionWidget
{
    private:
        QVBoxLayout _layout;
        QVBoxLayout _layout_items;
        TAPRoomNameLabel _section_title;
        TAPButton _validate_button;
        TAPLabel    _no_items_label;
        // items list : canonical_id, human_readable_name
        std::map<std::string, std::string> _items;
        std::vector<QRadioButton *> _item_buttons;

    public:
        TAPInventoryView();
        void update_inventory(std::map<std::string, std::string> data);
        std::string get_selected_item_id(void) const;
};

#endif // !TAP_INVENTORY_VIEW_HPP