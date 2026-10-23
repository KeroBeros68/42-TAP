#include "tap_exit_view.hpp"
#include "globals.hpp"

TAPExitView::TAPExitView(Session &session)
    : _exit_buttons_widget(session)
{
    // Label configuration
    this->_label_exits.setText(DEFAULT_EXIT_SECTION_LABEL_NAME);
    this->_no_exit_label.setText("(no available exit)");

    this->_layout.addWidget(&this->_label_exits, 0, Qt::AlignCenter);
    this->_layout.addWidget(&this->_exit_buttons_widget);
    this->_layout.addWidget(&this->_no_exit_label, 0, Qt::AlignCenter);
    this->setLayout(&this->_layout);
};

void TAPExitView::update_available_exits(std::map<std::string, std::string> data)
{
    if (data.size() == 0)
    {
        this->_no_exit_label.show();
        this->_exit_buttons_widget.hide();
    }
    else {
        this->_no_exit_label.hide();
        this->_exit_buttons_widget.show();
    }

    this->_exit_buttons_widget.update_available_exits(data);
};
