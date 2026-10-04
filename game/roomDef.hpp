#ifndef ROOMDEF_HPP
# define ROOMDEF_HPP

# include <string>
# include <unordered_map>
# include <vector>

class RoomDef {
	private:
		std::string _id;
		std::string _name;
		std::string _description;

		std::unordered_map<std::string, std::string> _exits;
		std::vector<std::string> _items;

	public:
		RoomDef(const std::string& id, const std::string& name, const std::string& description,
			const std::unordered_map<std::string, std::string>& exits, const std::vector<std::string>& items);

		const std::string& id() const;
		const std::string& name() const;
		const std::string& description() const;
		const std::unordered_map<std::string, std::string>& exits() const;
};

#endif