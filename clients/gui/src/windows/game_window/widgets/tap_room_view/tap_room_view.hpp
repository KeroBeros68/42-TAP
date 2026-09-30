#ifndef TAP_ROOM_VIEW_HPP
# define TAP_ROOM_VIEW_HPP

# include "../../../../custom_ui_elements/custom_ui_elements.hpp"
# include <QWidget>

// Widget displaying room info : name, description, items, npcs, exits
class TAPRoomView : public TAPGameSectionWidget
{
    private:
        QVBoxLayout *_layout = new QVBoxLayout();
        TAPLabel *_room_name = new TAPRoomNameLabel();
        TAPLabel *_room_description = new TAPLabel();
    
    public:
        TAPRoomView();

        void update_room_name(std::string name);
        void update_room_description(std::string description);
};

#endif // !TAP_ROOM_VIEW_HPP