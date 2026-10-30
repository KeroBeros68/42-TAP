//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../shared/protocol/chat.hpp"
# include "../../shared/protocol/line.hpp"
# include "commands.hpp"

//##############################################################################
//#                                  HELPERS                                   #
//##############################################################################

/**
 * CONNECT: chooses the name of the player and adds him to the game.
 * The name must be valid and not already taken.
 *
 * @param game the game
 * @param user the user of the connection
 * @param name the name wanted
 * @return     OK connected, or BAD_REQUEST / NAME_IN_USE
 */
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

/**
 * QUIT: removes the player from the game and closes the connection.
 *
 * @param game the game
 * @param user the user of the connection
 * @return     OK bye
 */
static Response cmdQuit(Game& game, User& user) {
	game.removePlayer(user.id());
	Response bye = Response::success(TapOk::BYE);
	bye.close = true;
	return bye;
}

/**
 * LOOK: describes the room of the player.
 *
 * @param game the game
 * @param user the user of the connection
 * @return     OK and the description, or BAD_REQUEST
 */
static Response cmdLook(Game& game, User& user) {
	const std::optional<LookData> look = game.look(user.id());
	if (!look)
		return Response::failure(TapError::BAD_REQUEST);
	return Response::success(TapOk::DATA, serializeLook(*look));
}

/**
 * MOVE: moves the player through an exit.
 * It is refused during a fight.
 *
 * @param game      the game
 * @param user      the user of the connection
 * @param direction north, east, south or west
 * @return          OK room=<id>, or IN_COMBAT / NO_EXIT
 */
static Response cmdMove(Game& game, User& user, const std::string& direction) {
	if (game.inCombat(user.id()))
		return Response::failure(TapError::IN_COMBAT);
	if (!game.move(user.id(), direction))
		return Response::failure(TapError::NO_EXIT);
	return Response::success(TapOk::ROOM, game.playerMap(user.id()));
}

/**
 * USE: uses an item of the inventory: it is consumed and gives its effect (hit points back).
 * It is refused during a fight.
 *
 * @param game   the game
 * @param user   the user of the connection
 * @param target designates the item
 * @return       OK healed=<hit points>, or BAD_REQUEST (no target) / IN_COMBAT / ITEM_NOT_IN_INVENTORY / ITEM_NOT_USABLE
 */
static Response cmdUse(Game& game, User& user, const std::string& target) {
	if (target.empty())
		return Response::failure(TapError::BAD_REQUEST);
	if (game.inCombat(user.id()))
		return Response::failure(TapError::IN_COMBAT);

	const UseResult used = game.useItem(user.id(), target);
	switch (used.status) {
		case UseStatus::USED:			return Response::success(TapOk::HEALED, std::to_string(used.healed));
		case UseStatus::NOT_FOUND:		return Response::failure(TapError::ITEM_NOT_IN_INVENTORY);
		case UseStatus::NOT_USABLE:		return Response::failure(TapError::ITEM_NOT_USABLE);
		case UseStatus::UNKNOWN_PLAYER:	break;
	}
	return Response::failure(TapError::BAD_REQUEST);
}

/**
 * TALK: makes the player talk to an npc of his room.
 * It is refused during a fight.
 *
 * @param game   the game
 * @param user   the user of the connection
 * @param target designates the npc
 * @return       OK and a line the npc says, or IN_COMBAT / NPC_NOT_FOUND
 */
static Response cmdTalk(Game& game, User& user, const std::string& target) {
	if (game.inCombat(user.id()))
		return Response::failure(TapError::IN_COMBAT);
	const std::optional<std::string> dialogue = game.talk(user.id(), target);
	if (!dialogue)
		return Response::failure(TapError::NPC_NOT_FOUND);
	return Response::success(TapOk::DATA, *dialogue);
}

/**
 * TAKE: takes an item from the floor.
 * It is refused during a fight.
 *
 * @param game   the game
 * @param user   the user of the connection
 * @param target designates the item
 * @return       OK taken=<id>, or IN_COMBAT / INVENTORY_FULL / ITEM_NOT_FOUND
 */
static Response cmdTake(Game& game, User& user, const std::string& target) {
	if (game.inCombat(user.id()))
		return Response::failure(TapError::IN_COMBAT);
	const TransferResult taken = game.takeItem(user.id(), target);
	if (taken.status == TransferStatus::DestinationFull)
		return Response::failure(TapError::INVENTORY_FULL);
	if (taken.status == TransferStatus::NotFound)
		return Response::failure(TapError::ITEM_NOT_FOUND);
	return Response::success(TapOk::TAKEN, taken.definition_id);
}

/**
 * DROP: drops an item of the player on the floor.
 *
 * @param game   the game
 * @param user   the user of the connection
 * @param target designates the item
 * @return       OK dropped=<id>, or ITEM_NOT_IN_INVENTORY
 */
static Response cmdDrop(Game& game, User& user, const std::string& target) {
	const TransferResult dropped = game.dropItem(user.id(), target);
	if (dropped.status != TransferStatus::Moved)
		return Response::failure(TapError::ITEM_NOT_IN_INVENTORY);
	return Response::success(TapOk::DROPPED, dropped.definition_id);
}

/**
 * Turns the report of ATTACK, DEFEND or FLEE into a response.
 *
 * @param report what happened
 * @return       OK and the turns, or the error that says why nothing happened
 */
static Response combatResponse(const CombatReport& report) {
	switch (report.status) {
		case CombatStatus::NOT_FOUND:		return Response::failure(TapError::NPC_NOT_FOUND);
		case CombatStatus::NOT_HOSTILE:		return Response::failure(TapError::NPC_NOT_HOSTILE);
		case CombatStatus::ANOTHER_ENEMY:	return Response::failure(TapError::IN_COMBAT);
		case CombatStatus::NOT_IN_COMBAT:	return Response::failure(TapError::NOT_IN_COMBAT);
		case CombatStatus::NOT_YOUR_TURN:	return Response::failure(TapError::NOT_YOUR_TURN);
		case CombatStatus::DONE:
		case CombatStatus::QUEUED:			break;
	}
	return Response::success(TapOk::DATA, serializeAttack(report));
}

/**
 * ATTACK: attacks an enemy.
 *
 * @param game   the game
 * @param user   the user of the connection
 * @param target the enemy, empty during a fight
 * @return       the response of combatResponse
 */
static Response cmdAttack(Game& game, User& user, const std::string& target) {
	return combatResponse(game.attack(user.id(), target));
}

/**
 * DEFEND: plays a defense in the fight in progress.
 *
 * @param game the game
 * @param user the user of the connection
 * @return     the response of combatResponse
 */
static Response cmdDefend(Game& game, User& user) {
	return combatResponse(game.defend(user.id()));
}

/**
 * FLEE: tries to run away from the fight in progress.
 *
 * @param game the game
 * @param user the user of the connection
 * @return     the response of combatResponse
 */
static Response cmdFlee(Game& game, User& user) {
	return combatResponse(game.flee(user.id()));
}

/**
 * INVENTORY: lists the items of the player. It is allowed during a fight.
 *
 * @param game the game
 * @param user the user of the connection
 * @return     OK and a JSON array of the ids of his items, or BAD_REQUEST
 */
static Response cmdInventory(Game& game, User& user) {
	const std::optional<std::vector<std::string>> ids = game.showInventories(user.id());
	if (!ids)
		return Response::failure(TapError::BAD_REQUEST);
	return Response::success(TapOk::DATA, serializeInventory(*ids));
}

/**
 * STATUS: tells the hit points and the state of the fight of the player.
 *
 * @param game the game
 * @param user the user of the connection
 * @return     OK and the status, or BAD_REQUEST
 */
static Response cmdStatus(Game& game, User& user) {
	const Stats* stats = game.status(user.id());
	if (!stats)
		return Response::failure(TapError::BAD_REQUEST);
	return Response::success(TapOk::DATA, serializeStatus(*stats, game.combatView(user.id())));
}

/**
 * WHO: tells how many players are in the game. It is allowed during a fight.
 *
 * @param game the game
 * @return     OK players=<count>
 */
static Response cmdWho(const Game& game) {
	return Response::success(TapOk::PLAYERS, std::to_string(game.playerCount()));
}

/**
 * CHAT: sends a message to everybody, to the players of the room, or to the players of the group.
 * It is allowed during a fight.
 *
 * @param game the game
 * @param user the user of the connection
 * @param args the scope (GLOBAL, ROOM or GROUP, in any case), a space, then the message
 * @return     OK, or BAD_REQUEST (unknown scope, empty or invalid message) / NOT_IN_GROUP
 */
static Response cmdChat(Game& game, User& user, const std::string& args) {
	std::string word, text;
	splitCommand(args, word, text);
	for (char& c : word)
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

	ChatScope scope;
	if (word == CHAT_SCOPE_GLOBAL)
		scope = ChatScope::GLOBAL;
	else if (word == CHAT_SCOPE_ROOM)
		scope = ChatScope::ROOM;
	else if (word == CHAT_SCOPE_GROUP)
		scope = ChatScope::GROUP;
	else
		return Response::failure(TapError::BAD_REQUEST);
	if (!isValidChatMessage(text))
		return Response::failure(TapError::BAD_REQUEST);

	switch (game.chat(user.id(), scope, text)) {
		case ChatStatus::SENT:				return Response::success();
		case ChatStatus::NOT_IN_GROUP:		return Response::failure(TapError::NOT_IN_GROUP);
		case ChatStatus::UNKNOWN_PLAYER:	break;
	}
	return Response::failure(TapError::BAD_REQUEST);
}

/**
 * GROUP: creates a group, invites a player, joins the group of a player who invited, or leaves the group.
 * It is allowed during a fight.
 *
 * @param game the game
 * @param user the user of the connection
 * @param args the sub-command (CREATE, INVITE, JOIN or LEAVE, in any case), then its argument
 * @return     OK (with group=<id> for CREATE and JOIN), or NOT_IN_GROUP / ALREADY_IN_GROUP / PLAYER_NOT_FOUND / NO_INVITATION /
 *             NOT_GROUP_LEADER / GROUP_FULL / BAD_REQUEST (unknown sub-command or missing argument)
 */
static Response cmdGroup(Game& game, User& user, const std::string& args) {
	std::string word, target;
	splitCommand(args, word, target);
	for (char& c : word)
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

	GroupReply reply;
	bool with_id = false;
	if (word == GROUP_CREATE && target.empty()) {
		reply = game.groupCreate(user.id());
		with_id = true;
	} else if (word == GROUP_INVITE && !target.empty())
		reply = game.groupInvite(user.id(), target);
	else if (word == GROUP_JOIN && !target.empty()) {
		reply = game.groupJoin(user.id(), target);
		with_id = true;
	} else if (word == GROUP_LEAVE && target.empty())
		reply = game.groupLeave(user.id());
	else
		return Response::failure(TapError::BAD_REQUEST);

	if (reply.unknown_player)
		return Response::failure(TapError::PLAYER_NOT_FOUND);
	switch (reply.status) {
		case GroupStatus::DONE:				break;
		case GroupStatus::NOT_IN_GROUP:		return Response::failure(TapError::NOT_IN_GROUP);
		case GroupStatus::ALREADY_IN_GROUP:
		case GroupStatus::SELF:				return Response::failure(TapError::ALREADY_IN_GROUP);
		case GroupStatus::NOT_LEADER:		return Response::failure(TapError::NOT_GROUP_LEADER);
		case GroupStatus::GROUP_FULL:		return Response::failure(TapError::GROUP_FULL);
		case GroupStatus::NO_INVITATION:	return Response::failure(TapError::NO_INVITATION);
	}
	return with_id ? Response::success(TapOk::GROUP, reply.group_id) : Response::success();
}

/**
 * QUEST: talks to an npc about quests. He takes the result of a quest in progress, or gives the quest he offers.
 * It is refused during a fight.
 *
 * @param game   the game
 * @param user   the user of the connection
 * @param target designates the npc
 * @return       OK and the quest, or BAD_REQUEST (no target) / IN_COMBAT / NPC_NOT_FOUND / NO_QUEST_AVAILABLE / INVENTORY_FULL
 */
static Response cmdQuest(Game& game, User& user, const std::string& target) {
	if (target.empty())
		return Response::failure(TapError::BAD_REQUEST);
	if (game.inCombat(user.id()))
		return Response::failure(TapError::IN_COMBAT);

	const QuestReply reply = game.quest(user.id(), target);
	switch (reply.status) {
		case QuestStatus::STARTED:
		case QuestStatus::IN_PROGRESS:
		case QuestStatus::COMPLETED:		return Response::success(TapOk::DATA, serializeQuest(reply));
		case QuestStatus::NONE:				return Response::failure(TapError::NO_QUEST_AVAILABLE);
		case QuestStatus::NPC_NOT_FOUND:	return Response::failure(TapError::NPC_NOT_FOUND);
		case QuestStatus::INVENTORY_FULL:	return Response::failure(TapError::INVENTORY_FULL);
		case QuestStatus::UNKNOWN_PLAYER:	break;
	}
	return Response::failure(TapError::BAD_REQUEST);
}

/**
 * QUESTS: lists the quests of the player, in progress and finished. It is allowed during a fight.
 *
 * @param game the game
 * @param user the user of the connection
 * @return     OK and a JSON array of the quests, or BAD_REQUEST
 */
static Response cmdQuests(Game& game, User& user) {
	const std::optional<std::vector<QuestView>> quests = game.quests(user.id());
	if (!quests)
		return Response::failure(TapError::BAD_REQUEST);
	return Response::success(TapOk::DATA, serializeQuests(*quests));
}

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

void registerCommands(Server& server, Game& game) {
	server.defineAction(CMD_CONNECT, [&game](User& user, const std::string& name) {
		return cmdConnect(game, user, name);
	});
	server.defineAction(CMD_WHO, [&game](User&, const std::string&) {
		return cmdWho(game);
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
	server.defineAction(CMD_USE, [&game](User& user, const std::string& target) {
		return cmdUse(game, user, target);
	});
	server.defineAction(CMD_DROP, [&game](User& user, const std::string& target) {
		return cmdDrop(game, user, target);
	});

	server.defineAction(CMD_ATTACK, [&game](User& user, const std::string& target) {
		return cmdAttack(game, user, target);
	});
	server.defineAction(CMD_DEFEND, [&game](User& user, const std::string&) {
		return cmdDefend(game, user);
	});
	server.defineAction(CMD_FLEE, [&game](User& user, const std::string&) {
		return cmdFlee(game, user);
	});
	server.defineAction(CMD_INVENTORY, [&game](User& user, const std::string&) {
		return cmdInventory(game, user);
	});
	server.defineAction(CMD_CHAT, [&game](User& user, const std::string& args) {
		return cmdChat(game, user, args);
	});
	server.defineAction(CMD_QUEST, [&game](User& user, const std::string& target) {
		return cmdQuest(game, user, target);
	});
	server.defineAction(CMD_QUESTS, [&game](User& user, const std::string&) {
		return cmdQuests(game, user);
	});
	server.defineAction(CMD_GROUP, [&game](User& user, const std::string& args) {
		return cmdGroup(game, user, args);
	});
	server.defineAction(CMD_STATUS, [&game](User& user, const std::string&) {
		return cmdStatus(game, user);
	});

}
