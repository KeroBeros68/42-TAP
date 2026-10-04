#ifndef ROOMDB_HPP
# define ROOMDB_HPP

# include <string>
# include <unordered_map>

# include "roomDef.hpp"

class RoomDb {
	private:
		std::unordered_map<std::string, RoomDef> _rooms;

	public:
		bool addRoom(const RoomDef& room);
		const RoomDef& room(const std::string& id) const;
		const std::unordered_map<std::string, RoomDef>& rooms() const;
};

#endif
