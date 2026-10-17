#ifndef DEFINITION_HPP
# define DEFINITION_HPP

# include <string>
# include <unordered_map>

class Definition {
	private:
		std::string _id;
		std::string _name;
		std::string _description;

	public:
		Definition(const std::string& id, const std::string& name, const std::string& description)
			: _id(id), _name(name), _description(description) {}

		const std::string& id() const {return _id;}
		const std::string& name() const {return _name;}
		const std::string& description() const {return _description;}
};

template <typename Def, typename Object>
class Database {
	private:
		std::unordered_map<std::string, Def> _definitions;

	public:
		bool add(const Def& def) {return _definitions.emplace(def.id(), def).second;}
		const Def& get(const std::string& id) const {return _definitions.at(id);}
		const std::unordered_map<std::string, Def>& all() const {return _definitions;}
		Object instantiate(const std::string& id) const {return Object(get(id));}
};

#endif