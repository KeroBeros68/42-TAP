#include "tap_exit_view.hpp"
#include "../../../../../globals.hpp"

TAPExitView::TAPExitView()
{
    // Label configuration
    this->_label_exits.setText(DEFAULT_EXIT_SECTION_LABEL_NAME);
    
    this->_layout.addWidget(&this->_label_exits, 0, Qt::AlignCenter);
    this->_layout.addWidget(&this->_exit_buttons_widget, 0, Qt::AlignCenter);
    this->setLayout(&this->_layout);
};

void TAPExitView::update_available_exits(std::map<std::string, std::string> data)
{
    this->_exit_buttons_widget.update_available_exits(data);
};
