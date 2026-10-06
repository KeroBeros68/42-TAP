#include "tap_connection_window.hpp"
#include <iostream>
#include "../../../globals.hpp"
#include "../../custom_ui_elements/custom_ui_elements.hpp"
#include <QWidget>
#include <string>

// findSystemUsername
std::string TAPConnectionWindow::findSystemUsername()
{
    char file_content[255]; // buffer
    FILE *name; // file containing username
    name = popen("whoami", "r");
    fgets(file_content, sizeof(file_content), name);
    file_content[strlen(file_content) - 1] = '\0'; // remove \n
    return std::string(file_content);
}

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
    std::cout << "[WIP IMPLEMENTATION]" << std::endl;
};

// getRemoteAddress
std::string TAPConnectionWindow::getRemoteAddress()
{
    return this->_address_filed.text().toStdString();
};

// getRemotePort
std::string TAPConnectionWindow::getRemotePort()
{
    return this->_port_filed.text().toStdString();
};

// getUsername
std::string TAPConnectionWindow::getUsername()
{
    return this->_username_filed.text().toStdString();
};

// connectToServer
void TAPConnectionWindow::connectToServer()
{
    std::cout << "Connecting to " << this->getUsername() << "@" << this->getRemoteAddress() << ":" << this->getRemotePort() << std::endl;
}

// Init window with elements
TAPConnectionWindow::TAPConnectionWindow(std::string window_title, int width, int height)
{
    this->_username = findSystemUsername();

    // Apply title
	this->setWindowTitle(window_title.c_str());

    // Apply dimensions
	this->resize(width, height);

    // Create text block
    this->_info_text.setText("Enter server address and port :");

    // Create username field
	this->_username_filed.setPlaceholderText(this->_username.c_str());
	this->_username_filed.setText(this->_username.c_str());

    // Create address field
	this->_address_filed.setPlaceholderText(this->_remote_address.c_str());
	this->_address_filed.setText(this->_remote_address.c_str());

    // Create port fiels
	this->_port_filed.setPlaceholderText(this->_remote_port.c_str());
	this->_port_filed.setText(this->_remote_port.c_str());

    // Create button
	this->_button.setText("Connect");
	this->_button.setStyleSheet(BUTTON_PROPERTIES);

    // Create layout
	this->_layout.addStretch();
    this->_layout.addWidget(&_info_text, 0, Qt::AlignCenter);
	this->_layout.addWidget(&_username_filed, 0, Qt::AlignCenter);
	this->_layout.addWidget(&_address_filed, 0, Qt::AlignCenter);
	this->_layout.addWidget(&_port_filed, 0, Qt::AlignCenter);
	this->_layout.addWidget(&_button, 0, Qt::AlignCenter);
	this->_layout.addStretch();
	this->setLayout(&this->_layout);

    // Connect button
	this->connect(&this->_button,
        &QPushButton::clicked,
        this,
        [this] {
            this->connectToServer();
    });
}
