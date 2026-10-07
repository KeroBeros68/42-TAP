#ifndef TAP_GROUND_ITEMS_VIEW_HPP
# define TAP_GROUND_ITEMS_VIEW_HPP

# include "globals.hpp"
# include "src/custom_ui_elements/custom_ui_elements.hpp"

class TAPGroundItemsView : public TAPGameSectionWidget
{
    private:
        // items list :     canonical_id, human_readable_name
        std::map<std::string, std::string>  items;

    public:
        TAPGroundItemsView();
        ~TAPGroundItemsView();
};

#endif // !TAP_GROUND_ITEMS_VIEW_HPP
