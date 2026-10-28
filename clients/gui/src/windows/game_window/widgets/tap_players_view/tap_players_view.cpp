#include "tap_players_view.hpp"

TAPPlayersView::TAPPlayersView()
{
    // Section title
    this->_section_title.setText("PLAYERS HERE");
    this->_layout.addWidget(&this->_section_title, 0, Qt::AlignCenter);

    // LOOK lists ourselves too, so this shows only before the first answer
    this->_no_players_label.setText("(no players here)");
    this->_layout.addWidget(&this->_no_players_label, 0, Qt::AlignCenter);
    this->_layout.addLayout(&this->_layout_players);

    this->setLayout(&this->_layout);

    // Empty room at startup
    this->update_players({});
}

void TAPPlayersView::update_players(std::vector<std::string> data)
{
    // Remove previous player labels
    for (auto *label : this->_player_labels)
    {
        delete label;
    }
    this->_player_labels.clear();

    this->_players = data;

    // One line per player, in the order the server sent them.
    // The names are displayed as received: unlike items and npcs, they do not
    // go through the Catalog, there is nothing to translate.
    for (auto &player : data) {
        auto *label = new TAPLabel();
        label->setParent(this);
        label->setText(QString::fromStdString(player));
        this->_layout_players.addWidget(label, 0, Qt::AlignCenter);
        this->_player_labels.push_back(label);
    }

    // Nobody is here
    if (data.size() == 0) {
        this->_no_players_label.show();
    }
    else {
        this->_no_players_label.hide();
    }
}
