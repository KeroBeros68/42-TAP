#include "tap_npc_talk.hpp"
#include <iostream>

TAPNPCTalkView::TAPNPCTalkView(Session &session)
    : _session(session)
{
    // Section title
    this->_section_title.setText("NPC TALK");
    this->_layout.addWidget(&this->_section_title, 0, Qt::AlignCenter);

    this->_no_items_label.setText("(no npcs available)");
    this->_layout.addWidget(&this->_no_items_label, 0, Qt::AlignCenter);
    this->_layout.addLayout(&this->_layout_items);

    this->_validate_button.setText("TALK");
    this->_quest_button.setText("QUEST");
    this->_attack_button.setText("ATTACK");
    this->_defend_button.setText("DEFEND");
    this->_flee_button.setText("FLEE");
    this->_layout.addStretch();
    // TALK and QUEST side by side
    this->_talk_quest_layout.addWidget(&this->_validate_button);
    this->_talk_quest_layout.addWidget(&this->_quest_button);
    this->_layout.addLayout(&this->_talk_quest_layout);
    // Combat actions side by side, under ATTACK
    this->_combat_buttons_layout.addWidget(&this->_attack_button);
    this->_combat_buttons_layout.addWidget(&this->_defend_button);
    this->_combat_buttons_layout.addWidget(&this->_flee_button);
    this->_layout.addLayout(&this->_combat_buttons_layout);
    this->connect(&this->_validate_button,
        &QPushButton::clicked,
        this,
        [this] {
            if (this->get_selected_npc_id() == "") {return ;}
            this->_session.send("TALK", this->get_selected_npc_id());
    });
    this->connect(&this->_quest_button,
        &QPushButton::clicked,
        this,
        [this] {
            if (this->get_selected_npc_id() == "") {return ;}
            this->_session.send("QUEST", this->get_selected_npc_id());
    });
    this->connect(&this->_attack_button,
        &QPushButton::clicked,
        this,
        [this] {
            if (this->get_selected_npc_id() == "") {return ;}
            this->_session.send("ATTACK", this->get_selected_npc_id());
    });
    this->connect(&this->_defend_button,
        &QPushButton::clicked,
        this,
        [this] {
            // DEFEND takes no target: the fight is already the selected one
            this->_session.send("DEFEND");
    });
    this->connect(&this->_flee_button,
        &QPushButton::clicked,
        this,
        [this] {
            this->_session.send("FLEE");
    });

    this->setLayout(&this->_layout);

    // Empty ground at startup
    this->update_npcs({});

    // Debug example
    // this->update_npcs(std::map<std::string, std::string> {
    //     {"npc.kevin_bertrand_cheronnet", "Kevin Bertrand Cheronnet"},
    //     {"npc.laurent", "Laurent"},
    // });
}

void TAPNPCTalkView::update_npcs(std::vector<ListedItem> data)
{
    // Genocide previous buttons
    for (auto *btn: this->_npc_buttons)
    {
        delete btn;
    }
    this->_npc_buttons.clear();

    this->_npcs = data;

    for (auto &npc : data) {
        auto *btn = new QRadioButton(QString::fromStdString(npc.name), this);
        btn->setProperty("canonical_id", QString::fromStdString(npc.id));
        btn->setStyleSheet(TAP_LABEL_PROPERTIES);
        this->_layout_items.addWidget(btn);
        this->_npc_buttons.push_back(btn);
    }

    // No items are present
    if (data.size() == 0) {
        this->_no_items_label.show();
        this->_validate_button.hide();
        this->_quest_button.hide();
        this->_attack_button.hide();
        this->_defend_button.hide();
        this->_flee_button.hide();
    }
    else {
        this->_no_items_label.hide();
        this->_validate_button.show();
        this->_quest_button.show();
        this->_attack_button.show();
        this->_defend_button.show();
        this->_flee_button.show();
    }

}

std::string TAPNPCTalkView::get_selected_npc_id(void) const
{
    for (auto *btn : this->_npc_buttons)
        if (btn->isChecked())
            return btn->property("canonical_id").toString().toStdString();
    return "";  // rien de sélectionné
}
