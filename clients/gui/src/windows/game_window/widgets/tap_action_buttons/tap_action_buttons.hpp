#ifndef TAP_ACTION_BUTTONS_HPP
# define TAP_ACTION_BUTTONS_HPP

# include "../../../../custom_ui_elements/custom_ui_elements.hpp"
# include "../../../../../../state/session.hpp"
# include <iostream>
# include <QWidget>
# include <QVBoxLayout>

class TAPActionButtons : public TAPGameSectionWidget
{
    private:
        QVBoxLayout _layout;
        TAPButton   _buttons[12];
        Session     &_session;

    public:
        explicit TAPActionButtons(Session &session);
};

#endif // !TAP_ACTION_BUTTONS_HPP
