#include "game.hpp"

#include <iostream>

Game::Game() {}

void Game::addPlayer(const User& user) {
	Player p;
	p.name = user.name();
	p.current_map = _world.start_room;
	_players[user.id()] = p;

	auto room = _world.rooms.find(p.current_map);
	if (room == _world.rooms.end()) {
		std::cerr << "[WARN] player " << user.id() << ": unknown map '" << p.current_map << "'" << std::endl;
		return;
	}
	room->second.addPlayer(user.id());
}

void Game::removePlayer(long long id) {
	auto player = _players.find(id);
	if (player == _players.end())
		return;

	auto room = _world.rooms.find(player->second.current_map);
	if (room != _world.rooms.end())
		room->second.removePlayer(id);
	_players.erase(player);
}

bool Game::hasPlayer(long long id) const {
	return _players.find(id) != _players.end();
}

bool Game::isNameTaken(const std::string& name) const {
	for (const auto& p : _players) {
		if (p.second.name == name)
			return true;
	}
	return false;
}

size_t Game::playerCount() const {
	return _players.size();
}

std::vector<long long> Game::playerIds() const {
	std::vector<long long> ids;
	ids.reserve(_players.size());
	for (const auto& p : _players)
		ids.push_back(p.first);
	return ids;
}

const World& Game::world() const {
	return _world;
}

const std::string& Game::playerMap(long long id) const {
	return _players.at(id).current_map;
}

std::string Game::look(long long player_id) const {
	const std::string& map = _players.at(player_id).current_map;

	std::vector<std::string> names;
	for (long long id : _world.rooms.at(map).currentPlayer()) {
		auto player = _players.find(id);
		if (player != _players.end())
			names.push_back(player->second.name);
	}
	return _world.look(map, names);
}
