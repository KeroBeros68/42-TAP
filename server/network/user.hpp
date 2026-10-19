#ifndef USER_HPP
# define USER_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                    USER                                    #
//##############################################################################

class User {
	private:
		//######################### VARIABLES ##########################
		long long	_id;
		std::string	_name;
		bool		_authenticated = false;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Builds the user of a new connection, who has no name yet.
		 *
		 * @param id id of the connection
		 */
		explicit User(long long id);

		//######################### FUNCTIONS ##########################
		/** Returns the id of the user (the one of his connection and of his player). */
		long long id() const;

		/** Returns the name of the user (empty before CONNECT). */
		const std::string& name() const;

		/**
		 * Chooses the name of the user, once.
		 *
		 * @param name the name, which must be valid (see isValidPlayerName)
		 * @return     false if the user already has a name or the name is not valid
		 */
		bool setName(const std::string& name);

		/** Tells whether the user did CONNECT. */
		bool isAuthenticated() const;

		/** Marks the user as connected. */
		void authenticate();
};

#endif
