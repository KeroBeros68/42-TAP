#include "tap_quests_view.hpp"
#include <iostream>

TAPQuestsView::TAPQuestsView()
{
    // Section title
    this->_section_title.setText("QUESTS");
    this->_layout.addWidget(&this->_section_title, 0, Qt::AlignCenter);

    this->_no_quests_label.setText("(no active quests)");
    this->_layout.addWidget(&this->_no_quests_label, 0, Qt::AlignCenter);
    this->_layout.addLayout(&this->_layout_items);
    this->_layout.addStretch();

    this->setLayout(&this->_layout);

    // Empty quests at startup
    this->update_quests({});

    // Debug example
    // this->update_quests(std::vector<std::map<std::string, std::string>> {
    //     {
    //         {"quest_id", "fetch_shield"},
    //         {"status", "active"},
    //         {"progress", "1/3"},
    //     },
    // });
}

void TAPQuestsView::update_quests(std::vector<std::map<std::string, std::string>> data)
{
    // Remove previous quest labels
    for (auto *label : this->_quest_labels)
    {
        delete label;
    }
    this->_quest_labels.clear();

    // Each quest: {"quest_id", "..."}, {"status", "..."}, {"progress", "..."}
    for (auto &quest : data) {
        auto *label = new TAPLabel();
        label->setParent(this);
        label->setText(QString::fromStdString(quest["quest_id"])
            + "  [" + QString::fromStdString(quest["status"]) + "]  "
            + QString::fromStdString(quest["progress"]));
        this->_layout_items.addWidget(label);
        this->_quest_labels.push_back(label);
    }

    // No quests are present
    if (data.size() == 0) {
        this->_no_quests_label.show();
    }
    else {
        this->_no_quests_label.hide();
    }
}
