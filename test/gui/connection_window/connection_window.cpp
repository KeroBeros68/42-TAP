#include "connection_window.hpp"
#include <iostream>
#include "../../globals.hpp"

// setRemoteAddress
void TAPConnectionWindow::setRemoteAddress(std::string address)
{
    this->_remote_address = address;
    std::cout << this->_remote_address << std::endl;
};

// getRemoteAddress
std::string TAPConnectionWindow::getRemoteAddress()
{
    return this->_remote_address;
};

TAPConnectionWindow::TAPConnectionWindow(std::string window_title, int width, int height) {
    // Apply title
	this->setWindowTitle(window_title.c_str());

    // Apply dimensions
	this->resize(width, height);

    // Change background color
    std::string style = "background-color: ";
    style += BG_COLOR;
    this->setStyleSheet(style.c_str());

    // Create elements
	QPushButton *button = new QPushButton("Connect"); // Create connect button
	QLineEdit *address_filed = new QLineEdit(); // Create text area for IP address
	button->setStyleSheet("background-color: #97c5a5; color: white; border-radius: 5px; padding: 5px;");
	address_filed->setPlaceholderText(this->_remote_address.c_str());
	address_filed->setText(this->_remote_address.c_str());

    // Create layout
    QVBoxLayout *layout = new QVBoxLayout();
	layout->addStretch();
	layout->addWidget(address_filed, 0, Qt::AlignCenter);
	layout->addWidget(button, 0, Qt::AlignCenter);
	layout->addStretch();
	this->setLayout(layout);

    // Connect button
	this->connect(button, &QPushButton::clicked, this, [this, address_filed]() {this->setRemoteAddress(address_filed->text().toStdString());}); // Set button usage
}
