#include "roomDb.hpp"

bool RoomDb::addRoom(const RoomDef& room) {
	return _rooms.emplace(room.id(), room).second;
}

const RoomDef& RoomDb::room(const std::string& id) const {
	return _rooms.at(id);
}

const std::unordered_map<std::string, RoomDef>& RoomDb::rooms() const {
	return _rooms;
}
