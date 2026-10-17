#include "tap_action_buttons.hpp"
#include "globals.hpp"

TAPActionButtons::TAPActionButtons()
{
    // List of actions available according to the subject (V.4)
    const char *actions[12] = {
        "LOOK", "MOVE", "TAKE", "DROP", "TALK", "ATTACK",
        "STATUS", "QUEST", "QUESTS", "WHO", "GROUP", "QUIT"
    };

    // Apply style to each button, and add them to the layout
    for (int i = 0; i < 12; i++)
    {
        this->_buttons[i].setStyleSheet(BUTTON_PROPERTIES);
        this->_buttons[i].setText(actions[i]);
        this->_layout.addWidget(&this->_buttons[i]);
    }

    this->setLayout(&this->_layout);

    // Connect each button to print a WIP message with its action
    for (int i = 0; i < 12; i++)
    {
        const char *action = actions[i];
        this->connect(&this->_buttons[i],
            &QPushButton::clicked,
            this,
            [this, action] {
                std::cout << "[WIP] " << action << std::endl;
        });
    }
};
