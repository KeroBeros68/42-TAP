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

		std::vector<long long> _current_player;

	public:
		Room(const std::string& id, const std::string& name, const std::string& description,
			const std::unordered_map<std::string, std::string>& exits, const std::vector<std::string>& items);

		const std::string& id() const;
		const std::string& name() const;
		const std::string& description() const;
		const std::unordered_map<std::string, std::string>& exits() const;

		const std::vector<std::string>& items() const;

		const std::vector<long long>& currentPlayer() const;
		void addPlayer(long long id);
		void removePlayer(long long id);
};

std::string oppositeDirection(const std::string& direction);

using RoomMap = std::unordered_map<std::string, Room>;

#endif