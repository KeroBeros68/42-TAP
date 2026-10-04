#ifndef ITEMDEF_HPP
#define ITEMDEF_HPP

#include <string>

class ItemDef {
	private:
		std::string _id;
		std::string _name;
		std::string _description;
		bool _obtainable;

	public:
		ItemDef(const std::string& id, const std::string& name, const std::string& description, bool obtainable);

		const std::string& id() const;
		const std::string& name() const;
		const std::string& description() const;
		bool isObtainable() const;
};
#endif