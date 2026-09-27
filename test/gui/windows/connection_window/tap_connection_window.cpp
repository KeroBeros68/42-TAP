#include "tap_connection_window.hpp"
#include <iostream>
#include "../../../globals.hpp"
#include "../../custom_ui_elements/custom_ui_elements.hpp"

// setRemoteAddress
void TAPConnectionWindow::setRemoteAddress(std::string address)
{
    this->_remote_address = address;
    std::cout << "Server address has been set to : " << this->_remote_address << std::endl;
};

// setRemotePort
void TAPConnectionWindow::setRemotePort(std::string port)
{
    this->_remote_port= port;
    std::cout << "Server port has been set to : " << this->_remote_port << std::endl;
};

// getRemoteAddress
std::string TAPConnectionWindow::getRemoteAddress()
{
    return this->_remote_address;
};

// getRemotePort
std::string TAPConnectionWindow::getRemotePort()
{
    return this->_remote_port;
};

// Init window with elements
TAPConnectionWindow::TAPConnectionWindow(std::string window_title, int width, int height)
{
    // Apply title
	this->setWindowTitle(window_title.c_str());

    // Apply dimensions
	this->resize(width, height);

    // Create text block
    TAPLabel *info_text = new TAPLabel();
    info_text->setText("Enter server address and port :");

    // Create address field
	TAPLineEdit *address_filed = new TAPLineEdit(); // Create text area for IP address
	address_filed->setPlaceholderText(this->_remote_address.c_str());
	address_filed->setText(this->_remote_address.c_str());

    // Create port fiels
	TAPLineEdit *port_filed = new TAPLineEdit(); // Create text area for IP address
	port_filed->setPlaceholderText(this->_remote_port.c_str());
	port_filed->setText(this->_remote_port.c_str());

    // Create button
	QPushButton *button = new QPushButton("Connect"); // Create connect button
	button->setStyleSheet(VALIDATE_BUTTON_PROPERTIES);

    // Create layout
    QVBoxLayout *layout = new QVBoxLayout();
	layout->addStretch();
    layout->addWidget(info_text, 0, Qt::AlignCenter);
	layout->addWidget(address_filed, 0, Qt::AlignCenter);
	layout->addWidget(port_filed, 0, Qt::AlignCenter);
	layout->addWidget(button, 0, Qt::AlignCenter);
	layout->addStretch();
	this->setLayout(layout);

    // Connect button
	this->connect(button, &QPushButton::clicked, this, [this, address_filed, port_filed]() {
        this->setRemoteAddress(address_filed->text().toStdString()); // Set address
        this->setRemotePort(port_filed->text().toStdString()); // Set port
    });
}
