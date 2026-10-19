#ifndef DEFINITION_HPP
# define DEFINITION_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>
# include <unordered_map>

//##############################################################################
//#                                 DEFINITION                                 #
//##############################################################################

class Definition {
	private:
		//######################### VARIABLES ##########################
		std::string _id;
		std::string _name;
		std::string _description;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Builds a definition, what an npc or an item is before it is created.
		 *
		 * @param id          id of the definition, like "item.herbs"
		 * @param name        name shown to the players
		 * @param description description shown to the players
		 */
		Definition(const std::string& id, const std::string& name, const std::string& description)
			: _id(id), _name(name), _description(description) {}

		//########################## GETTERS ###########################
		/** Returns the id of the definition. */
		const std::string& id() const {return _id;}

		/** Returns the name of the definition. */
		const std::string& name() const {return _name;}

		/** Returns the description of the definition. */
		const std::string& description() const {return _description;}
};

//##############################################################################
//#                                  DATABASE                                  #
//##############################################################################

template <typename Def, typename Object>
class Database {
	private:
		//######################### VARIABLES ##########################
		std::unordered_map<std::string, Def> _definitions;

	public:
		//######################### FUNCTIONS ##########################
		/**
		 * Adds a definition.
		 *
		 * @param def the definition to add
		 * @return    false if a definition with the same id is already there
		 */
		bool add(const Def& def) {return _definitions.emplace(def.id(), def).second;}

		/**
		 * Finds a definition.
		 *
		 * @param id id of the definition
		 * @return   the definition (std::out_of_range is thrown if there is none)
		 */
		const Def& get(const std::string& id) const {return _definitions.at(id);}

		/** Returns all the definitions, by id. */
		const std::unordered_map<std::string, Def>& all() const {return _definitions;}

		/**
		 * Creates an object from the definition of an id.
		 *
		 * @param id id of the definition
		 * @return   the new npc or item (std::out_of_range is thrown if there is no such definition)
		 */
		Object instantiate(const std::string& id) const {return Object(get(id));}
};

#endif
