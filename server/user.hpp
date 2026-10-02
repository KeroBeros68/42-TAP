#ifndef USER_HPP
# define USER_HPP

# include <string>

// Everything the game knows about a player. No network data here.
struct User {
	long long	id;
	std::string	name;
	bool		authenticated = false;

	User(long long id);
};

#endif
