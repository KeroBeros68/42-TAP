#ifndef PARSER_HPP
# define PARSER_HPP

# include <string>

# include "npc.hpp"
# include "room.hpp"

void parseNpcs(const std::string& path, NpcDb& npcs);
void parseRooms(const std::string& path, RoomMap& rooms, std::string& start_room, std::string& respawn_room);

#endif
