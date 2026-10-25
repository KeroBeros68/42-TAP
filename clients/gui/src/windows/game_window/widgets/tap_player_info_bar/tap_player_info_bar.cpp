#include "tap_player_info_bar.hpp"

TAPPlayerInfoBar::TAPPlayerInfoBar()
{
    // Default texts, replaced as soon as the server answers STATUS
    this->_label_name.setText(DEFAULT_PLAYER_NAME_MESSAGE);
    this->_label_status.setText(DEFAULT_PLAYER_STATUS_UNKNOWN_MESSAGE);

    // The bar fills as the player gets hit points back. 0/100 until STATUS
    // answers: showing a full bar would claim a health the server never sent.
    this->_hp_bar.setRange(0, 100);
    this->_hp_bar.setValue(0);
    this->_hp_bar.setFormat("%v / %m");
    this->_hp_bar.setStyleSheet(PLAYER_HP_BAR_PROPERTIES);
    // Capped so the stretch below keeps some width on a wide window: without
    // it the bar absorbs all the slack and the name sticks to the status.
    this->_hp_bar.setMaximumWidth(PLAYER_HP_BAR_MAX_WIDTH);

    // [<player>]                 [<status>]  [hp bar]
    // The stretch is what keeps the name and the status apart: without it the
    // three widgets are packed and read as one sentence ("Player : alice wounded").
    this->_layout.addWidget(&this->_label_name, 0, Qt::AlignCenter);
    this->_layout.addStretch();
    this->_layout.addWidget(&this->_label_status, 0, Qt::AlignCenter);
    this->_layout.addWidget(&this->_hp_bar, 1);

    this->setLayout(&this->_layout);
}

void TAPPlayerInfoBar::update_player_info(const std::string &name, int hp,
    int max_hp, const std::string &status)
{
    this->_label_name.setText(
        (std::string(DEFAULT_PLAYER_NAME_MESSAGE) + name).c_str()
    );

    // The status word comes from the server (STATUS_* in shared/defines.hpp).
    // It is shown as sent: the client never decides by itself that we are dead.
    this->_label_status.setText(status.empty()
        ? DEFAULT_PLAYER_STATUS_UNKNOWN_MESSAGE
        : status.c_str());

    // A maximum of 0 means STATUS never answered: keep the bar empty rather
    // than divide by zero or draw a full one.
    if (max_hp <= 0) {
        this->_hp_bar.setRange(0, 100);
        this->_hp_bar.setValue(0);
        this->_hp_bar.setFormat(DEFAULT_PLAYER_HP_UNKNOWN_MESSAGE);
        return;
    }

    this->_hp_bar.setRange(0, max_hp);
    this->_hp_bar.setValue(hp < 0 ? 0 : (hp > max_hp ? max_hp : hp));
    this->_hp_bar.setFormat("%v / %m");
}
