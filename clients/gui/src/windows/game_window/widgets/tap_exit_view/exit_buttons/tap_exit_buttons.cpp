#include "tap_exit_buttons.hpp"
# include "../../../../../custom_ui_elements/custom_ui_elements.hpp"

TAPExitButtons::TAPExitButtons()
{
    // Apply style each to button, and add them to the layout
    for (int i = 0; i < 4; i++)
    {
        this->_buttons[i].setStyleSheet(BUTTON_PROPERTIES);
        this->_buttons[i].hide();
        this->_layout.addWidget(&this->_buttons[i], 0, Qt::AlignCenter);
        this->_layout.addStretch();
    }

    this->setLayout(&this->_layout);

    // this->update_available_exits(ex);
};

void TAPExitButtons::update_available_exits(std::map<std::string, std::string> data)
{
    // North update
    if (data["north"] != "")
    {
        this->_buttons[0].setText((std::string("North : ") + data["north"]).c_str());
        this->_buttons[0].show();
    }
    else
        this->_buttons[0].hide();

    // South update
    if (data["south"] != "")
    {
        this->_buttons[1].setText((std::string("South : ") + data["south"]).c_str());
        this->_buttons[1].show();
    }
    else
        this->_buttons[1].hide();

    // East update
    if (data["east"] != "")
    {
        this->_buttons[2].setText((std::string("East : ") + data["east"]).c_str());
        this->_buttons[2].show();
    }
    else
        this->_buttons[2].hide();

    // West update
    if (data["west"] != "")
    {
        this->_buttons[3].setText((std::string("West : ") + data["west"]).c_str());
        this->_buttons[3].show();
    }
    else
        this->_buttons[3].hide();
};
