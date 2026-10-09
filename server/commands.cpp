#include "commands.hpp"

static Response cmdConnect(Game& game, User& user, const std::string& name) {
	if (!user.setName(name)) {
		LOG_WARN("invalid player name", {"client", user.id()}, {"name", name});
		return Response::failure(TapError::BAD_REQUEST);
	}
	if (game.isNameTaken(name)) {
		LOG_INFO("player name already in use", {"client", user.id()}, {"name", name});
		return Response::failure(TapError::NAME_IN_USE);
	}
	user.authenticate();
	game.addPlayer(user);
	LOG_INFO("player connected", {"client", user.id()}, {"player", user.name()});
	return Response::success(TapOk::CONNECTED);
}

static Response cmdQuit(Game& game, User& user) {
	game.removePlayer(user.id());
	LOG_INFO("player disconnected", {"client", user.id()}, {"player", user.name()});
	user.authenticate();
	return Response::success(TapOk::BYE);
}

static Response cmdLook(Game& game, User& user) {
	if (!game.hasPlayer(user.id()))
		return Response::failure(TapError::BAD_REQUEST);
	return Response::success(TapOk::DATA, game.look(user.id()));
}

static Response cmdMove(Game& game, User& user, const std::string& direction) {
	if (!game.move(user.id(), direction))
		return Response::failure(TapError::NO_EXIT);
	return Response::success(TapOk::ROOM, game.playerMap(user.id()));
}

// Placeholder for the commands that are not implemented yet
static Response cmdStub(const std::string& cmd, User& user, const std::string& args) {
	LOG_DEBUG("command not implemented", {"client", user.id()}, {"cmd", cmd}, {"args", args});
	return Response::success(TapOk::DATA, "Message received");
}

void registerCommands(Server& server, Game& game) {
	server.defineAction(CMD_CONNECT, [&game](User& user, const std::string& name) {
		return cmdConnect(game, user, name);
	});
	server.defineAction(CMD_LOOK, [&game](User& user, const std::string&) {
		return cmdLook(game, user);
	});
	server.defineAction(CMD_MOVE, [&game](User& user, const std::string& dir) {
		return cmdMove(game, user, dir);
	});
	server.defineAction(CMD_QUIT, [&game](User& user, const std::string&) {
		return cmdQuit(game, user);
	});


	// Replace an entry by a real handler (like cmdConnect) once the command is implemented
	const std::string stubs[] = {
		CMD_CHAT, CMD_TAKE, CMD_DROP, CMD_INVENTORY, CMD_TALK,
		CMD_ATTACK, CMD_STATUS, CMD_QUEST, CMD_QUESTS, CMD_WHO, CMD_GROUP
	};
	for (const std::string& cmd : stubs) {
		server.defineAction(cmd, [cmd](User& user, const std::string& args) {
			return cmdStub(cmd, user, args);
		});
	}
}
