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
	game.addPlayer(user.id(), user.name());
	LOG_INFO("player connected", {"client", user.id()}, {"player", user.name()});
	return Response::success(TapOk::CONNECTED);
}

static Response cmdQuit(Game& game, User& user) {
	game.removePlayer(user.id());
	Response bye = Response::success(TapOk::BYE);
	bye.close = true;
	return bye;
}

static Response cmdLook(Game& game, User& user) {
	const std::optional<LookData> look = game.look(user.id());
	if (!look)
		return Response::failure(TapError::BAD_REQUEST);
	return Response::success(TapOk::DATA, serializeLook(*look));
}

static Response cmdMove(Game& game, User& user, const std::string& direction) {
	if (!game.move(user.id(), direction))
		return Response::failure(TapError::NO_EXIT);
	return Response::success(TapOk::ROOM, game.playerMap(user.id()));
}

static Response cmdTalk(Game& game, User& user, const std::string& target) {
	const std::optional<std::string> dialogue = game.talk(user.id(), target);
	if (!dialogue)
		return Response::failure(TapError::NPC_NOT_FOUND);
	return Response::success(TapOk::DATA, *dialogue);
}

static Response cmdTake(Game& game, User& user, const std::string& target) {
	const TransferResult taken = game.take(user.id(), target);
	if (taken.status == TransferStatus::DestinationFull)
		return Response::failure(TapError::INVENTORY_FULL);
	if (taken.status == TransferStatus::NotFound)
		return Response::failure(TapError::ITEM_NOT_FOUND);
	return Response::success(TapOk::TAKEN, taken.id);
}

static Response cmdDrop(Game& game, User& user, const std::string& target) {
	const TransferResult dropped = game.drop(user.id(), target);
	if (dropped.status != TransferStatus::Moved)
		return Response::failure(TapError::ITEM_NOT_IN_INVENTORY);
	return Response::success(TapOk::DROPPED, dropped.id);
}

static Response cmdAttack(Game& game, User& user, const std::string& target) {
	const CombatResult combat = game.attack(user.id(), target);
	if (combat.status == AttackStatus::NOT_FOUND)
		return Response::failure(TapError::NPC_NOT_FOUND);
	if (combat.status == AttackStatus::NOT_HOSTILE)
		return Response::failure(TapError::NPC_NOT_HOSTILE);
	if (combat.status == AttackStatus::DEAD)
		return Response::failure(TapError::BAD_REQUEST);	
	return Response::success(TapOk::DATA, serializeAttack(combat));
}

static Response cmdStatus(Game& game, User& user) {
	const Stats* stats = game.status(user.id());
	if (!stats)
		return Response::failure(TapError::BAD_REQUEST);
	return Response::success(TapOk::DATA, serializeStatus(*stats));
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
	server.defineAction(CMD_TALK, [&game](User& user, const std::string& npc_id) {
		return cmdTalk(game, user, npc_id);
	});
	server.defineAction(CMD_TAKE, [&game](User& user, const std::string& target) {
		return cmdTake(game, user, target);
	});
	server.defineAction(CMD_DROP, [&game](User& user, const std::string& target) {
		return cmdDrop(game, user, target);
	});

	server.defineAction(CMD_ATTACK, [&game](User& user, const std::string& target) {
		return cmdAttack(game, user, target);
	});
	server.defineAction(CMD_STATUS, [&game](User& user, const std::string&) {
		return cmdStatus(game, user);
	});
	// Replace an entry by a real handler (like cmdConnect) once the command is implemented
	const std::string stubs[] = {
		CMD_CHAT, CMD_INVENTORY,
		CMD_QUEST, CMD_QUESTS, CMD_WHO, CMD_GROUP
	};
	for (const std::string& cmd : stubs) {
		server.defineAction(cmd, [cmd](User& user, const std::string& args) {
			return cmdStub(cmd, user, args);
		});
	}
}
