#include "tap_error_window.hpp"
#include "globals.hpp"
#include "../../custom_ui_elements/custom_ui_elements.hpp"

// Constructor
TAPErrorWindow::TAPErrorWindow()
{
    // Window properties
    this->setWindowTitle(ERROR_WINDOW_TITLE);
    this->resize(ERROR_WINDOW_WIDTH, ERROR_WINDOW_HEIGHT);

    // Default error message
    this->setErrorMessage(ERROR_WINDOW_DEFAULT_MESSAGE);

	_layout.addStretch();
    _layout.addWidget(&_error_text_label, 0, Qt::AlignCenter);
	_layout.addStretch();
	this->setLayout(&_layout);
};

// setErrorMessage
void TAPErrorWindow::setErrorMessage(std::string message)
{
    this->_error_text_label.setText(message.c_str());
};

// getErrorMessage
std::string TAPErrorWindow::getErrorMessage()
{
    return this->_error_text_label.text().toStdString();
};
