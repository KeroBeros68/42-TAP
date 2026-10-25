#include "tap_inventory_view.hpp"
#include "globals.hpp"
#include <iostream>

TAPInventoryView::TAPInventoryView(Session &session)
    : _session(session)
{
    // Section title
    this->_section_title.setText("INVENTORY");
    this->_layout.addWidget(&this->_section_title, 0, Qt::AlignCenter);

    this->_no_items_label.setText("(empty inventory)");
    this->_layout.addWidget(&this->_no_items_label, 0, Qt::AlignCenter);
    this->_layout.addLayout(&this->_layout_items);

    this->_validate_button.setText("DROP");
    this->_layout.addStretch();
    this->_layout.addWidget(&this->_validate_button);
        this->connect(&this->_validate_button,
            &QPushButton::clicked,
            this,
            [this] {
                if (this->get_selected_item_id() == "") {return ;}
                this->_session.send("DROP", this->get_selected_item_id());
        });

    this->setLayout(&this->_layout);

    // Empty inventory at startup
    this->update_inventory({});

    // // Debug example : uncomment to display items in the inventory
    // this->update_inventory(std::map<std::string, std::string> {
    //     {"item.kevin_bouclier#1", "kevin bouclier"},
    //     {"item.kevin_sword#2", "kevin sword"},
    // });
}

void TAPInventoryView::update_inventory(std::vector<ListedItem> data)
{
    // Genocide previous buttons
    for (auto *btn: this->_item_buttons)
    {
        delete btn;
    }
    this->_item_buttons.clear();

    this->_items = data;

    for (auto &item : data) {
        auto *btn = new QRadioButton(QString::fromStdString(item.name), this);
        btn->setProperty("canonical_id", QString::fromStdString(item.id));
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

std::string TAPInventoryView::get_selected_item_id(void) const
{
    for (auto *btn : this->_item_buttons)
        if (btn->isChecked())
            return btn->property("canonical_id").toString().toStdString();
    return "";  // rien de sélectionné
}