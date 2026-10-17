#ifndef TAP_ACTION_BUTTONS_HPP
# define TAP_ACTION_BUTTONS_HPP

# include "../../../../custom_ui_elements/custom_ui_elements.hpp"
# include <iostream>
# include <QWidget>
# include <QVBoxLayout>

class TAPActionButtons : public TAPGameSectionWidget
{
    private:
        QVBoxLayout _layout;
        TAPButton   _buttons[12];

    public:
        TAPActionButtons();
};

#endif // !TAP_ACTION_BUTTONS_HPP
