//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "user.hpp"
# include "../../shared/player_name.hpp"
# include "../defines.hpp"

//##############################################################################
//#                                    USER                                    #
//##############################################################################

//######################## CONSTRUCTOR #########################

User::User(long long id) : _id(id) {}

//######################### FUNCTIONS ##########################

long long User::id() const {
	return _id;
}

const std::string& User::name() const {
	return _name;
}

bool User::setName(const std::string& name) {
	if (_authenticated || !isValidPlayerName(name))
		return false;
	_name = name;
	return true;
}

bool User::isAuthenticated() const {
	return _authenticated;
}

void User::authenticate() {
	_authenticated = true;
}
