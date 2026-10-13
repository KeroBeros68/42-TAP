#include "tap_button.hpp"
#include "globals.hpp"

TAPButton::TAPButton()
{
    this->setStyleSheet(BUTTON_PROPERTIES);
    this->setText("(No text set)");
};