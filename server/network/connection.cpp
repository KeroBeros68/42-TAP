//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "connection.hpp"
# include "../../shared/line.hpp"

//##############################################################################
//#                                 CONNECTION                                 #
//##############################################################################

//######################## CONSTRUCTOR #########################

Connection::Connection(int fd, const std::string& ip) : _fd(fd), _ip(ip) {}

//########################## IDENTITY ##########################

int Connection::fd() const {
	return _fd;
}

const std::string& Connection::ip() const {
	return _ip;
}

//########################### INPUT ############################

const std::string& Connection::in() const {
	return _in;
}

void Connection::appendIn(const char* data, size_t size) {
	_in.append(data, size);
}

bool Connection::popLine(std::string& line) {
	return extractLine(_in, line);
}

void Connection::clearIn() {
	_in.clear();
}

//########################### OUTPUT ###########################

const std::string& Connection::out() const {
	return _out;
}

void Connection::appendOut(const std::string& data) {
	_out += data;
}

void Connection::consumeOut(size_t count) {
	_out.erase(0, count);
}

//########################### STATE ############################

void Connection::setClosing(bool closing) {
	_closing = closing;
}

bool Connection::isClosing() const {
	return _closing;
}
