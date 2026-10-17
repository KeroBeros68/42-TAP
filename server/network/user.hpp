#ifndef USER_HPP
# define USER_HPP

# include <string>

class User {
	private:
		long long	_id;
		std::string	_name;
		bool		_authenticated = false;

	public:
		explicit User(long long id);

		long long id() const;
		const std::string& name() const;
		bool setName(const std::string& name);
		bool isAuthenticated() const;
		void authenticate();
};

#endif
