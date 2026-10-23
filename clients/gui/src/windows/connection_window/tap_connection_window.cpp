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

// onStateChanged
//  The Session updates the state; this window only reads it.
//  Two moments matter here: the authentication succeeded (enter the game),
//  and an error arrived while we were waiting (show it, let the player retry).
void TAPConnectionWindow::onStateChanged(Section section)
{
    if (section != Section::Connection)
        return;

    const GameState &state = this->_session.state();

    // "OK connected": the name was accepted, enter the game
    if (this->_awaiting_connect
        && state.phase == GameState::Phase::AUTHENTICATED)
    {
        this->_awaiting_connect = false;
        this->_game_window.show();
        this->hide();
        return;
    }

    // The CONNECT was refused: the server closed nothing, but we give up
    // on this attempt and let the player change a field and try again
    if (this->_awaiting_connect && state.last_error.has_value())
    {
        this->_awaiting_connect = false;

        // "201 NAME_IN_USE": a friendlier wording for the only error we know
        std::string message = "The server refused the connection: "
            + *state.last_error;
        if (state.last_error->find("NAME_IN_USE") != std::string::npos)
            message = "This player name is already in use.";
        this->_client.disconnect();
        this->_error_window.setErrorMessage(message);
        this->_error_window.show();
    }
};

// connectToServer
void TAPConnectionWindow::connectToServer()
{
    std::cout << "Connecting to " << this->getUsername() << "@" << this->getRemoteAddress() << ":" << this->getRemotePort() << "..." << std::endl;
    _client.setServer(getRemoteAddress(), std::atoi(getRemotePort().c_str()));

    // Server connection
    try {
        _client.connect();
        this->_session.restart(); // a new game: no leftover from the last one
        this->_awaiting_connect = true;
        this->_session.send("CONNECT", this->getUsername());
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
    Client &client, Session &session, TAPErrorWindow &err_window, TAPGameWindow &game_window)
    : _client(client), _session(session), _error_window(err_window), _game_window(game_window)
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

    // The lines are read by the Session (main.cpp calls session.listen()).
    // main.cpp also owns the single onChanged slot, and calls onStateChanged()
    // on this window: there is only one slot, so a window must not take it.

    // Connect button
	this->connect(&this->_button,
        &QPushButton::clicked,
        this,
        [this] {
            this->connectToServer();
    });
}
