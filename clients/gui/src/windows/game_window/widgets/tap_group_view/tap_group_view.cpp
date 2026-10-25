#include "tap_group_view.hpp"
#include <iostream>

TAPGroupView::TAPGroupView(Session &session)
    : _session(session)
{
    // Section title
    this->_section_title.setText("GROUP");
    this->_layout.addWidget(&this->_section_title, 0, Qt::AlignCenter);

    // Group status and invitation
    this->_layout.addWidget(&this->_status_label, 0, Qt::AlignCenter);
    this->_layout.addWidget(&this->_invite_label, 0, Qt::AlignCenter);

    // INVITE <player> : the name is typed, so a field and a button side by side
    this->_invite_field.setPlaceholderText("player name");
    this->_invite_button.setText("INVITE");
    this->_invite_row.addWidget(&this->_invite_field);
    this->_invite_row.addWidget(&this->_invite_button);
    this->_layout.addLayout(&this->_invite_row);

    // JOIN <leader> : prefilled with the player who invited us, still editable
    this->_join_field.setPlaceholderText("leader name");
    this->_join_button.setText("JOIN");
    this->_join_row.addWidget(&this->_join_field);
    this->_join_row.addWidget(&this->_join_button);
    this->_layout.addLayout(&this->_join_row);

    // CREATE and LEAVE take no argument
    this->_create_button.setText("CREATE");
    this->_leave_button.setText("LEAVE");
    this->_actions_row.addWidget(&this->_create_button);
    this->_actions_row.addWidget(&this->_leave_button);
    this->_layout.addLayout(&this->_actions_row);

    this->_layout.addStretch();
    this->setLayout(&this->_layout);

    this->connect(&this->_invite_button,
        &QPushButton::clicked,
        this,
        [this] {
            if (this->_invite_field.text().isEmpty()) {return ;}
            this->_session.send("GROUP", "INVITE " + this->_invite_field.text().toStdString());
    });
    this->connect(&this->_join_button,
        &QPushButton::clicked,
        this,
        [this] {
            if (this->_join_field.text().isEmpty()) {return ;}
            this->_session.send("GROUP", "JOIN " + this->_join_field.text().toStdString());
    });
    this->connect(&this->_create_button,
        &QPushButton::clicked,
        this,
        [this] {
            this->_session.send("GROUP", "CREATE");
    });
    this->connect(&this->_leave_button,
        &QPushButton::clicked,
        this,
        [this] {
            this->_session.send("GROUP", "LEAVE");
    });

    // Nobody is in a group at startup
    this->update_group(false, "", "");
}

void TAPGroupView::update_group(bool in_group, const std::string &group_id,
    const std::string &pending_invite)
{
    // Where we stand: the id of the group, or nothing
    this->_status_label.setText(in_group
        ? QString::fromStdString("in " + group_id)
        : "(no group)");

    // An invitation names its leader: it is the argument of GROUP JOIN
    if (pending_invite.empty()) {
        this->_invite_label.hide();
    }
    else {
        this->_invite_label.setText(QString::fromStdString(pending_invite + " invites you"));
        this->_invite_label.show();
        this->_join_field.setText(QString::fromStdString(pending_invite));
    }

    // CREATE is refused when we already have a group, JOIN when we have one
    // too, LEAVE when we have none: only show what the server would accept
    this->_create_button.setEnabled(!in_group);
    this->_leave_button.setEnabled(in_group);
    this->_join_button.setEnabled(!in_group);
}
