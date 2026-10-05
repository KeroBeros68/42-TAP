#include "commands.hpp"

static Response cmdConnect(Game& game, User& user, const std::string& name) {
	if (!user.setName(name))
		return Response::failure(TapError::BAD_REQUEST);
	if (game.isNameTaken(name))
		return Response::failure(TapError::NAME_IN_USE);
	user.authenticate();
	game.addPlayer(user);
	std::cout << "User " << user.id() << " connected as " << user.name() << " on " << game.playerMap(user.id()) << std::endl;
	return Response::success(TapOk::CONNECTED);
}

static Response cmdLook(Game& game, User& user) {
	if (!game.hasPlayer(user.id()))
		return Response::failure(TapError::BAD_REQUEST);
	return Response::success(TapOk::DATA, game.look(user.id()));
}

static Response cmdMove(Game& game, User& user, const std::string& direction) {
	if (!game.move(user.id(), direction))
		return Response::failure(TapError::NO_EXIT);
	std::string res = "room=";
	res.append(game.playerMap(user.id()));
	return Response::success(TapOk::DATA, res);
}

// Placeholder for the commands that are not implemented yet
static Response cmdStub(const std::string& cmd, User& user, const std::string& args) {
	std::cout << "Received " << cmd << " message from user " << user.id() << std::endl;
	std::cout << "Message content: " << args << std::endl;
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


	// Replace an entry by a real handler (like cmdConnect) once the command is implemented
	const std::string stubs[] = {
		CMD_CHAT, CMD_TAKE, CMD_DROP, CMD_INVENTORY, CMD_TALK,
		CMD_ATTACK, CMD_STATUS, CMD_QUEST, CMD_QUESTS, CMD_WHO, CMD_GROUP, CMD_QUIT
	};
	for (const std::string& cmd : stubs) {
		server.defineAction(cmd, [cmd](User& user, const std::string& args) {
			return cmdStub(cmd, user, args);
		});
	}
}
