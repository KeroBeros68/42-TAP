#ifndef INSTANCE_HPP
# define INSTANCE_HPP

# include <string>
# include "../../shared/defines.hpp"

template <typename Def>
class Instance {
	private:
		static inline int _counter = 0;
		std::string _id;
		const Def* _definition;

	public:
		explicit Instance(const Def& def)
			: _id(def.id() + INSTANCE_SEPARATOR + std::to_string(++_counter)), _definition(&def) {}
		const std::string& id() const {return _id;}
		const Def& definition() const {return *_definition;}
		const std::string& name() const {return _definition->name();}
};

#endif