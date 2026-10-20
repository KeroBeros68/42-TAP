#ifndef GAME_STATE_HPP
# define GAME_STATE_HPP

# include <string>
# include <map>
# include <vector>

class GameState {
    public:
        std::string                         id; // Room id
        std::string                         name; // Room display name
        std::string                         description; // Room description
        std::map<std::string, std::string>  exits; // Exits
        std::vector<std::string>            players; // Players in room
        std::vector<std::string>            items; // Ground items
        std::vector<std::string>            npcs; // NPCs
};

#endif // !GAME_STATE_HPP