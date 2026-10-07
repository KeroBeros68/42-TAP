#include "tap_label.hpp"
#include "globals.hpp"

TAPLabel::TAPLabel()
{
    this->setStyleSheet(TAP_LABEL_PROPERTIES);
    this->setText("(No text set)");
};