#include "tap_action_buttons.hpp"
#include "globals.hpp"

TAPActionButtons::TAPActionButtons(Session &session)
    : _session(session)
{
    // List of actions available according to the subject (V.4)
    const char *actions[3] = {
        "LOOK", "WHO", "QUIT"
    };

    // Apply style to each button, and add them to the layout
    for (int i = 0; i < 3; i++)
    {
        this->_buttons[i].setStyleSheet(BUTTON_PROPERTIES);
        this->_buttons[i].setText(actions[i]);
        this->_layout.addWidget(&this->_buttons[i]);
    }

    this->setLayout(&this->_layout);

    // Connect each button to send its action to the server
    for (int i = 0; i < 3; i++)
    {
        const char *action = actions[i];
        this->connect(&this->_buttons[i],
            &QPushButton::clicked,
            this,
            [this, action] {
                // GROUP alone means nothing to the server: the sub-command
                // depends on the screen, so it stays a plain command for now
                this->_session.send(std::string(action));
        });
    }
};
