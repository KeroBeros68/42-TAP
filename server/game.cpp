#include "game.hpp"

Game::Game() {}

void Game::addPlayer(const User& user) {
	Player p;
	p.name = user.name();
	_players[user.id()] = p;
}

void Game::removePlayer(long long id) {
	_players.erase(id);
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
