#ifndef TAP_EXIT_VIEW_HPP
# define TAP_EXIT_VIEW_HPP

# include "../../../../custom_ui_elements/custom_ui_elements.hpp"
# include "exit_buttons/tap_exit_buttons.hpp"

class TAPExitView : public TAPGameSectionWidget
{
    private:
        QVBoxLayout         _layout;
        TAPRoomNameLabel    _label_exits;
        TAPExitButtons      _exit_buttons_widget;


    public:
        TAPExitView();

        void update_available_exits(std::map<std::string, std::string> data);
};

#endif // !TAP_EXIT_VIEW_HPP