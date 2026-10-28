#ifndef TAP_ACTION_BUTTONS_HPP
# define TAP_ACTION_BUTTONS_HPP

# include "../../../../custom_ui_elements/custom_ui_elements.hpp"
# include "../../../../../../state/session.hpp"
# include <iostream>
# include <QWidget>
# include <QVBoxLayout>

// One button per action of the subject that takes no argument.
// Actions with a target live in the widget that owns the target.
# define ACTION_COUNT	6	// LOOK, INVENTORY, STATUS, QUESTS, WHO, QUIT

class TAPActionButtons : public TAPGameSectionWidget
{
    private:
        QVBoxLayout _layout;
        TAPButton   _buttons[ACTION_COUNT];
        Session     &_session;

    public:
        explicit TAPActionButtons(Session &session);
};

#endif // !TAP_ACTION_BUTTONS_HPP
