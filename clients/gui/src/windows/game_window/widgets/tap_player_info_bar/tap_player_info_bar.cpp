#include "tap_player_info_bar.hpp"
#include <algorithm>
#include <cstdio>

namespace {

// Bar colour from the hit point ratio: full green at 100%, red at 0%.
std::string hp_bar_style(double ratio)
{
    ratio = std::clamp(ratio, 0.2, 0.8);

    char color[8];
    std::snprintf(color, sizeof(color), "#%02x%02x00",
        static_cast<int>((1.0 - ratio) * 255.0),
        static_cast<int>(ratio * 255.0));

    return std::string(PLAYER_HP_BAR_CSS_PREFIX) + color
        + PLAYER_HP_BAR_CSS_SUFFIX;
}

} // namespace

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
    this->setStyleSheet(status == std::string("combat") ? DEFAULT_PLAYER_STATUS_ATTACK_COLOR_BG : GAME_SECTION_WIDGET_PROPERTIES);

    // A maximum of 0 means STATUS never answered: keep the bar empty rather
    // than divide by zero or draw a full one.
    if (max_hp <= 0) {
        this->_hp_bar.setRange(0, 100);
        this->_hp_bar.setValue(0);
        this->_hp_bar.setFormat(DEFAULT_PLAYER_HP_UNKNOWN_MESSAGE);
        // No ratio to colour from: fall back to the plain bar, or the attack
        // one when the status says we are fighting.
        this->_hp_bar.setStyleSheet(
            status == std::string("combat") ?
            PLAYER_HP_BAR_PROPERTIES_ATTACK :
            PLAYER_HP_BAR_PROPERTIES);
        return;
    }

    const int clamped_hp = hp < 0 ? 0 : (hp > max_hp ? max_hp : hp);
    const double ratio = static_cast<double>(clamped_hp)
        / static_cast<double>(max_hp);

    this->_hp_bar.setRange(0, max_hp);
    this->_hp_bar.setValue(clamped_hp);
    this->_hp_bar.setFormat("%v / %m");
    this->_hp_bar.setStyleSheet(hp_bar_style(ratio).c_str());
}
