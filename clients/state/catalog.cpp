#include "catalog.hpp"
#include "message.hpp"
# include "../../third_party/json.hpp"

#include <fstream>

Catalog::Catalog()
{
    this->loadFile("game/ressources/items.json");
    this->loadFile("game/ressources/npc.json");
    this->loadFile("game/ressources/quests.json");
    this->loadFile("game/ressources/room.json");
};

bool Catalog::loadFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return false; // File not opened : return false

    // parse(flux, callback, allow_exceptions=false)
    nlohmann::json data = nlohmann::json::parse(file, nullptr, false);
    if (data.is_discarded())
        return false; // JSON malformé

    data.

    return true;
};
