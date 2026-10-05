#ifndef ROOM_HPP
# define ROOM_HPP

# include <string>
# include <unordered_map>
# include <vector>

class Room {
	private:
		std::string _id;
		std::string _name;
		std::string _description;

		std::unordered_map<std::string, std::string> _exits;
		std::vector<std::string> _items;

	public:
		Room(const std::string& id, const std::string& name, const std::string& description,
			const std::unordered_map<std::string, std::string>& exits, const std::vector<std::string>& items);

		const std::string& id() const;
		const std::string& name() const;
		const std::string& description() const;
		const std::unordered_map<std::string, std::string>& exits() const;
};

// The direction of the way back of an exit ("north" -> "south"), "" if it is not a known direction
std::string oppositeDirection(const std::string& direction);

// The rooms of the world, by id
using RoomMap = std::unordered_map<std::string, Room>;

#endif