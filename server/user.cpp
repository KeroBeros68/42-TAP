#include "user.hpp"
#include "defines.hpp"

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
	for (unsigned char c : name) {
		if (c <= ' ' || c == 0x7F)
			return false;
	}
	_name = name;
	return true;
}

bool User::isAuthenticated() const {
	return _authenticated;
}

void User::authenticate() {
	if (!_name.empty())
		_authenticated = true;
}
