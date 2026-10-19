#ifndef INSTANCE_HPP
# define INSTANCE_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../shared/defines.hpp"

//##############################################################################
//#                                  INSTANCE                                  #
//##############################################################################

template <typename Def>
class Instance {
	private:
		//######################### VARIABLES ##########################
		static inline int _counter = 0;
		std::string _id;
		const Def* _definition;

	public:
		//######################## CONSTRUCTOR #########################
		/**
		 * Creates an instance of a definition, with an id made of the id of the definition and a counter.
		 *
		 * @param def the definition, which must outlive the instance
		 */
		explicit Instance(const Def& def)
			: _id(def.id() + INSTANCE_SEPARATOR + std::to_string(++_counter)), _definition(&def) {}

		//########################## GETTERS ###########################
		/** Returns the id of the instance, like "npc.spider#13". */
		const std::string& id() const {return _id;}

		/** Returns the definition of the instance. */
		const Def& definition() const {return *_definition;}

		/** Returns the name of the definition. */
		const std::string& name() const {return _definition->name();}
};

#endif
