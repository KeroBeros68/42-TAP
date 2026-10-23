#include "tap_ground_items_view.hpp"
#include <iostream>

TAPGroundItemsView::TAPGroundItemsView(Session &session)
    : _session(session)
{
    // Section title
    this->_section_title.setText("GROUND ITEMS");
    this->_layout.addWidget(&this->_section_title, 0, Qt::AlignCenter);

    this->_no_items_label.setText("(no ground items available)");
    this->_layout.addWidget(&this->_no_items_label, 0, Qt::AlignCenter);
    this->_layout.addLayout(&this->_layout_items);

    this->_validate_button.setText("TAKE");
    this->_layout.addStretch();
    this->_layout.addWidget(&this->_validate_button);
        this->connect(&this->_validate_button,
            &QPushButton::clicked,
            this,
            [this] {
                if (this->get_selected_item_id() == "") {return ;}
                this->_session.send("TAKE", this->get_selected_item_id());
        });

    this->setLayout(&this->_layout);

    // Empty ground at startup
    this->update_ground_items({});

    // // Debug example : uncomment to display items on the ground
    // this->update_ground_items(std::map<std::string, std::string> {
    //     {"item.kevin_bouclier#1", "kevin bouclier"},
    //     {"item.kevin_sword#2", "kevin sword"},
    // });
}

void TAPGroundItemsView::update_ground_items(std::map<std::string, std::string> data)
{
    // Genocide previous buttons
    for (auto *btn: this->_item_buttons)
    {
        delete btn;
    }
    this->_item_buttons.clear();

    this->_items = data;

    for (auto &[id, name] : data) {
        auto *btn = new QRadioButton(QString::fromStdString(name), this);
        btn->setProperty("canonical_id", QString::fromStdString(id));
        btn->setStyleSheet(TAP_LABEL_PROPERTIES);
        this->_layout_items.addWidget(btn);
        this->_item_buttons.push_back(btn);
    }

    // No items are present
    if (data.size() == 0) {
        this->_no_items_label.show();
        this->_validate_button.hide();
    }
    else {
        this->_no_items_label.hide();
        this->_validate_button.show();
    }

}

std::string TAPGroundItemsView::get_selected_item_id(void) const
{
    for (auto *btn : this->_item_buttons)
        if (btn->isChecked())
            return btn->property("canonical_id").toString().toStdString();
    return "";  // rien de sélectionné
}
