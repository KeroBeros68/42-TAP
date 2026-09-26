#include "connection_window.hpp"
#include <iostream>

// setRemoteAddress
void TAPConnectionWindow::setRemoteAddress()
{
    std::cout << "yeah !" << std::endl;
};

// getRemoteAddress
std::string TAPConnectionWindow::getRemoteAddress()
{
    return this->_remote_address;
};
