#include "tap_action_buttons.hpp"
#include "globals.hpp"

TAPActionButtons::TAPActionButtons(Session &session)
    : _session(session)
{
    // Every action of the subject (V.4) that takes no argument. The others
    // (MOVE, TAKE, DROP, TALK, ATTACK, QUEST, GROUP, CHAT...) need a target,
    // so they are carried by the widget that owns that target: the exit
    // buttons, the ground items, the inventory, the npc list, the group panel.
    const char *actions[ACTION_COUNT] = {
        "LOOK", "INVENTORY", "STATUS", "QUESTS", "WHO", "QUIT"
    };

    // Apply style to each button, and add them to the layout
    for (int i = 0; i < ACTION_COUNT; i++)
    {
        this->_buttons[i].setStyleSheet(BUTTON_PROPERTIES);
        this->_buttons[i].setText(actions[i]);
        this->_layout.addWidget(&this->_buttons[i]);
    }

    this->setLayout(&this->_layout);

    // Connect each button to send its action to the server
    for (int i = 0; i < ACTION_COUNT; i++)
    {
        const char *action = actions[i];
        this->connect(&this->_buttons[i],
            &QPushButton::clicked,
            this,
            [this, action] {
                this->_session.send(std::string(action));
        });
    }
};
