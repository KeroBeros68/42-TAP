#ifndef TAP_GROUND_ITEMS_VIEW_HPP
# define TAP_GROUND_ITEMS_VIEW_HPP

# include "globals.hpp"
# include "src/custom_ui_elements/custom_ui_elements.hpp"

class TAPGroundItemsView : public TAPGameSectionWidget
{
    private:
        QVBoxLayout _layout;
        QPushButton _validate_button;
        // items list : canonical_id, human_readable_name
        std::map<std::string, std::string> _items;

    public:
        TAPGroundItemsView();

        void update_ground_items(std::map<std::string, std::string> data);

        // canonical_id of the selected item, or "" if nothing is selected
        std::string get_selected_item_id(void) const;

    private:
        void rebuild_buttons(void);

    private slots:
        void on_validate_clicked(void);
};

#endif // !TAP_GROUND_ITEMS_VIEW_HPP
