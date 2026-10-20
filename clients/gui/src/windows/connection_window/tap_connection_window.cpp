#include "tap_connection_window.hpp"
#include <iostream>
#include "globals.hpp"
#include "../../custom_ui_elements/custom_ui_elements.hpp"
#include <QWidget>
#include <string>
#include "../../../../client.hpp"
#include "../error_window/tap_error_window.hpp"

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

// onServerOk
void TAPConnectionWindow::onServerOk(const std::string &args)
{
    // The greeting arrives right after connect(), before the answer to CONNECT
    if (args.rfind("hello", 0) == 0) {
        std::cout << "[OK] " << args << std::endl;
        return;
    }

    // "OK connected": the name was accepted, enter the game
    if (this->_awaiting_connect && args == "connected") {
        this->_awaiting_connect = false;
        std::cout << "[OK] " << args << std::endl;
        this->_game_window.show();
        this->hide();
        return;
    }

    std::cout << "[OK] " << args << std::endl;
};

// onServerErr
void TAPConnectionWindow::onServerErr(const std::string &args)
{
    std::cout << "[ERR] " << args << std::endl;

    // Before entering the game, an error can only be the answer to CONNECT
    if (!this->_awaiting_connect)
        return;
    this->_awaiting_connect = false;

    // args is "201 NAME_IN_USE": show the error, then let the player retry
    std::string message = "The server refused the connection: " + args;
    if (args.find("NAME_IN_USE") != std::string::npos)
        message = "This player name is already in use.";
    this->_client.disconnect();
    this->_error_window.setErrorMessage(message);
    this->_error_window.show();
};

// connectToServer
void TAPConnectionWindow::connectToServer()
{
    std::cout << "Connecting to " << this->getUsername() << "@" << this->getRemoteAddress() << ":" << this->getRemotePort() << "..." << std::endl;
    _client.setServer(getRemoteAddress(), std::atoi(getRemotePort().c_str()));

    // Server connection
    try {
        _client.connect();
        this->_awaiting_connect = true;
        _client.send(std::string("CONNECT ") + this->getUsername().c_str());
    }
    catch (const std::exception &e) {
        this->_error_window.setErrorMessage(e.what());
        this->_error_window.show();
        return;
    }
    // The answer is read by client.update() (see the QTimer in main.cpp)
    // and handled by onServerOk() / onServerErr()
}

// Init window with elements
TAPConnectionWindow::TAPConnectionWindow(std::string window_title, int width, int height,
    Client &client, TAPErrorWindow &err_window, TAPGameWindow &game_window)
    : _client(client), _error_window(err_window), _game_window(game_window)
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

    // Server answers: defineAction keeps one handler per type, so printing is done here too
	this->_client.defineAction("OK", [this](const std::string &args) {
		this->onServerOk(args);
	});
	this->_client.defineAction("ERR", [this](const std::string &args) {
		this->onServerErr(args);
	});

    // Connect button
	this->connect(&this->_button,
        &QPushButton::clicked,
        this,
        [this] {
            this->connectToServer();
    });
}
