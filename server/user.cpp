#include "user.hpp"
#include "defines.hpp"
#include "../shared/utf8.hpp"

User::User(long long id) : _id(id) {}

long long User::id() const {
	return _id;
}

const std::string& User::name() const {
	return _name;
}

bool User::setName(const std::string& name) {
	if (_authenticated || name.empty() || name.size() > MAX_NAME_LENGTH)
		return false;
	if (!validateUTF8(name))
		return false;
	if (name.find(CMD_SEPARATOR) != std::string::npos)
		return false;
	_name = name;
	return true;
}

bool User::isAuthenticated() const {
	return _authenticated;
}

void User::authenticate() {
	_authenticated = _authenticated ? false : true;
}
