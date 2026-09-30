#include "tap_room_view.hpp"
#include "../../../../../globals.hpp"

TAPRoomView::TAPRoomView()
{
    this->_room_name->setText(DEFAULT_ROOM_NAME);

    this->_room_description->setText(DEFAULT_ROOM_DESCRIPTION);
    this->_room_description->setWordWrap(true);
    this->_room_description->setAlignment(Qt::AlignCenter);

    _layout->addWidget(this->_room_name, 0, Qt::AlignCenter);
    _layout->addWidget(this->_room_description, 0, Qt::AlignCenter);
    this->setLayout(this->_layout);
};

void TAPRoomView::update_room_name(std::string name)
{
    this->_room_name->setText(name.c_str());
};

void TAPRoomView::update_room_description(std::string description)
{
    this->_room_description->setText(description.c_str());
};