//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "room.hpp"
# include "../../shared/defines.hpp"
# include "../utils/lookup.hpp"

//##############################################################################
//#                                    ROOM                                    #
//##############################################################################

//######################## CONSTRUCTOR #########################

Room::Room(const std::string& id, const std::string& name, const std::string& description,
	const std::unordered_map<std::string, std::string>& exits,
	const std::vector<Spawn>& item_spawns, const std::vector<Spawn>& npc_spawns
)
	: _id(id), _name(name), _description(description), _exits(exits), _item_spawns(item_spawns), _npc_spawns(npc_spawns) {}

//########################## IDENTITY ##########################

const std::string& Room::id() const {
	return _id;
}

const std::string& Room::name() const {
	return _name;
}

const std::string& Room::description() const {
	return _description;
}

const std::unordered_map<std::string, std::string>& Room::exits() const {
	return _exits;
}

//########################### SPAWNS ###########################

const std::vector<Spawn>& Room::itemSpawns() const {
	return _item_spawns;
}

const std::vector<Spawn>& Room::npcSpawns() const {
	return _npc_spawns;
}

//############################ NPCS ############################

std::vector<std::string> Room::npcs() const {
	return lookup::sortedIds(_alive_npcs);
}

const std::unordered_map<std::string, Npc>& Room::aliveNpcs() const {
	return _alive_npcs;
}

const Npc*  Room::findNpc(const std::string& query)  const {
	return lookup::findIn(_alive_npcs, query);
}

Npc*  Room::findNpc(const std::string& query) {
	return lookup::findIn(_alive_npcs, query);
}

void Room::addNpc(const Npc& npc) {
	_alive_npcs.emplace(npc.id(), npc);
}

void Room::removeNpc(Npc* npc) {
	_alive_npcs.erase(npc->id());
}

//########################### ITEMS ############################

std::vector<std::string> Room::items() const {
	return _inventory.ids();
}

Inventory& Room::inventory() {
	return _inventory;
}

const Inventory& Room::inventory() const {
	return _inventory;
}

//########################## PLAYERS ###########################

const std::vector<long long>& Room::currentPlayer() const {
	return _current_player;
}

void Room::addPlayer(long long id) {
	if (std::find(_current_player.begin(), _current_player.end(), id) == _current_player.end())
		_current_player.push_back(id);
}

void Room::removePlayer(long long id) {
	_current_player.erase(std::remove(_current_player.begin(), _current_player.end(), id), _current_player.end());
}

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

std::string oppositeDirection(const std::string& direction) {
	if (direction == DIR_NORTH)
		return DIR_SOUTH;
	if (direction == DIR_SOUTH)
		return DIR_NORTH;
	if (direction == DIR_EAST)
		return DIR_WEST;
	if (direction == DIR_WEST)
		return DIR_EAST;
	return "";
}
