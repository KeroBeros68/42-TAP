#include "tap_room_name_label.hpp"
#include "../../../globals.hpp"

TAPRoomNameLabel::TAPRoomNameLabel()
{
    std::string style = std::string(TAP_LABEL_PROPERTIES) + std::string(TAP_GAME_SECTION_NAME_PROPERTIES);
    this->setStyleSheet(style.c_str());
}