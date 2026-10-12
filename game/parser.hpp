#ifndef PARSER_HPP
# define PARSER_HPP

# include <string>
# include <vector>

# include "npc.hpp"
# include "item.hpp"
# include "room.hpp"

# define MAX_SPAWN_COUNT 100

void parseNpcs(const std::string& path, NpcDb& npcs);
void parseItems(const std::string& path, ItemDb& items);
void parseRooms(const std::string& path, RoomMap& rooms, std::string& start_room, std::string& respawn_room);

std::string serializeLook(const Room& room, const std::vector<std::string>& players, const std::vector<std::string>& npcs, const std::vector<std::string>& items);

#endif
