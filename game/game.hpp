#ifndef GAME_HPP
# define GAME_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <chrono>
# include <optional>
# include <string>
# include <unordered_map>
# include <vector>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../shared/protocol/event.hpp"
# include "combat/combat_host.hpp"
# include "combat/combat_manager.hpp"
# include "entities/player.hpp"
# include "events/event_bus.hpp"
# include "group/group_manager.hpp"
# include "quests/quest_manager.hpp"
# include "world/presence.hpp"
# include "world/respawn_queue.hpp"
# include "world/world.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Loot ---------------------------------------------------------------------
# define LOOT_SEPARATOR	","	// between the ids of a loot in the logs

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class ChatScope { GLOBAL, ROOM, GROUP };

enum class ChatStatus { SENT, NOT_IN_GROUP, UNKNOWN_PLAYER };

enum class UseStatus { USED, NOT_FOUND, NOT_USABLE, UNKNOWN_PLAYER };

enum class GroupAction { CREATE, INVITE, JOIN, LEAVE };

struct GroupReply {
	GroupStatus status = GroupStatus::DONE;
	std::string group_id{};
	bool unknown_player = false;
};

enum class QuestStatus { STARTED, IN_PROGRESS, COMPLETED, NONE, NPC_NOT_FOUND, INVENTORY_FULL, UNKNOWN_PLAYER };

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct UseResult {
	UseStatus	status = UseStatus::NOT_FOUND;
	std::string	id{};
	int			healed = 0;
};

struct QuestView {
	const QuestDef*	def = nullptr;
	QuestState		state = QuestState::ACTIVE;
	int				current = 0;
};

struct QuestReply {
	QuestStatus	status = QuestStatus::NONE;
	QuestView	view{};
};

struct LookData {
	const Room* room;
	std::vector<std::string> players;
};

//##############################################################################
//#                                    GAME                                    #
//##############################################################################

class Game : private CombatHost {
	private:
		//######################### VARIABLES ##########################
		World _world;
		std::unordered_map<long long, Player> _players;
		EventBus _events;
		CombatManager _combat;
		RespawnQueue _respawns;
		GroupManager _groups;
		QuestManager _quests;
		std::unordered_map<std::string, std::chrono::steady_clock::time_point> _effect_due;

		//########################## PRESENCE ##########################
		/**
		 * Finds a player and the room where he is.
		 *
		 * @param player_id id of the player
		 * @param action    what is asked, for the log if he is unknown
		 * @return          the player and his room (both null if one is unknown)
		 */
		ConstPresence locate(long long player_id, const char* action) const;

		/** @copydoc CombatHost::locate */
		Presence locate(long long player_id, const char* action) override;

		/** @copydoc CombatHost::findPlayer */
		Player* findPlayer(long long player_id) override;

		/** @copydoc CombatHost::findRoom */
		Room* findRoom(const std::string& room_id) override;

		/**
		 * Moves a player to another room, with no rule checked.
		 * The two rooms are told with PRESENCE events, and the player is not in combat any more.
		 *
		 * @param player_id id of the player
		 * @param target    id of the room
		 * @return          false if the player or the room is unknown
		 */
		bool teleport(long long player_id, const std::string& target) override;

		//########################### EVENTS ###########################
		/** @copydoc CombatHost::events */
		EventBus& events() override;

		/**
		 * Queues a PRESENCE event for the players of a room.
		 *
		 * @param room the room
		 * @param type EVT_PRESENCE_ENTER or EVT_PRESENCE_LEAVE
		 * @param name name of the player who came or left
		 */
		void emitPresence(const Room& room, const char* type, const std::string& name);

		/** Queues the number of players for everybody. */
		void emitPlayerCount();

		/**
		 * Queues a SPAWN event for the players of a room.
		 *
		 * @param room the room where something came back
		 * @param kind EVT_SPAWN_NPC or EVT_SPAWN_ITEM
		 * @param id   id of the object (the event carries the id of its definition)
		 */
		void emitSpawn(const Room& room, const char* kind, const std::string& id);

		/**
		 * Queues an ITEM event for the other players of a room, whose floor changed.
		 * The player who acted does not receive it: his answer already tells him.
		 *
		 * @param room          the room whose floor changed
		 * @param except_id     id of the player who took or dropped (he is skipped)
		 * @param action        EVT_ITEM_TAKEN or EVT_ITEM_DROPPED
		 * @param name          name of the player who took or dropped
		 * @param definition_id id of the definition of the item (the event never carries an instance id)
		 */
		void emitItem(const Room& room, long long except_id, const char* action,
			const std::string& name, const std::string& definition_id);

		//############################ QUESTS ##########################
		/**
		 * Counts what a player did for a quest: the items he carries (to bring) or the enemies he defeated.
		 *
		 * @param player the player
		 * @param quest  his progress
		 * @return       the number counted, at most the number asked
		 */
		int questCurrent(const Player& player, const QuestProgress& quest) const;

		/**
		 * Finishes a quest: the items are taken, the reward is given and the quest is marked as completed.
		 * Nothing changes if the reward does not fit in the inventory.
		 *
		 * @param player the player
		 * @param quest  his progress, which must be done
		 * @return       COMPLETED, or INVENTORY_FULL
		 */
		QuestStatus completeQuest(Player& player, QuestProgress& quest);

		//########################## EFFECTS ###########################
		/**
		 * Gives hit points to a player, up to his maximum. A player in a fight or dead is not healed.
		 *
		 * @param player_id id of the player
		 * @param amount    the hit points to give
		 * @return          the hit points he really got (0 if he is unknown, in a fight, dead or at full health)
		 */
		int regen(long long player_id, int amount);

		/**
		 * Queues an EFFECT event for a player healed by his room.
		 *
		 * @param player_id id of the player
		 * @param gained    the hit points he got
		 */
		void emitRegen(long long player_id, int gained);

		/**
		 * Queues an INVENTORY CHANGED event for the player whose inventory changed (he has to ask INVENTORY again).
		 *
		 * @param player_id id of the player
		 * @param reason    EVT_INVENTORY_QUEST or EVT_INVENTORY_USE
		 */
		void emitInventoryChanged(long long player_id, const char* reason);

		/**
		 * Applies the effects of the rooms whose time came: each player in such a room is healed.
		 *
		 * @param now the current time
		 */
		void applyRoomEffects(std::chrono::steady_clock::time_point now);

		//############################ GROUP ###########################
		/**
		 * Queues a GROUP event for some players.
		 *
		 * @param players_ids ids of the players who receive it
		 * @param type        EVT_GROUP_INVITE, EVT_GROUP_JOIN, EVT_GROUP_LEAVE or EVT_CHAT
		 * @param data        what follows the type
		 */
		void emitGroup(const std::vector<long long>& players_ids, const char* type, const std::string& data);

		/**
		 * Makes a player leave his group, and tells the members who stay.
		 *
		 * @param player_id id of the player
		 * @return          what GroupManager::leave returns
		 */
		GroupResult leaveGroup(long long player_id);

		//########################## RESPAWN ###########################
		/** @copydoc CombatHost::enemyDied */
		void enemyDied(Room& room, Npc* npc, const std::string& killer, const std::vector<long long>& participants) override;

		/** @copydoc CombatHost::respawnRoom */
		const std::string& respawnRoom() const override;

		/** @copydoc CombatHost::respawnLife */
		int respawnLife() const override;

	public:
		//######################## CONSTRUCTOR #########################
		/** Builds the game: the world is loaded from the data files. */
		Game();

		//########################## PLAYERS ###########################
		/**
		 * Adds a new player in the start room.
		 * The room and everybody are told (PRESENCE and player count events).
		 *
		 * @param id   id of the player (the one of his connection)
		 * @param name name of the player
		 */
		void addPlayer(long long id, const std::string& name);

		/**
		 * Removes a player (he left or his connection closed). He leaves his fight, if any.
		 * The room and everybody are told (PRESENCE and player count events).
		 *
		 * @param id id of the player
		 */
		void removePlayer(long long id);

		/** Tells whether a player is in the game. */
		bool hasPlayer(long long id) const;

		/** Tells whether a player already has this name. */
		bool isNameTaken(const std::string& name) const;

		/** Returns the number of players. */
		size_t playerCount() const;

		/** Lists the ids of all the players. */
		std::vector<long long> playerIds() const;

		/** Returns the stats of a player (nullptr if he is unknown). */
		const Stats* status(long long player_id) const;

		//########################### WORLD ############################
		/** Returns the world. */
		const World& world() const;

		/** Returns the id of the room where a player is (the player must exist). */
		const std::string& playerMap(long long id) const;

		/**
		 * Describes what a player sees: his room and the names of the players in it.
		 *
		 * @param player_id id of the player
		 * @return          what he sees (nothing if he is unknown)
		 */
		std::optional<LookData> look(long long player_id) const;

		/**
		 * Moves a player through an exit of his room.
		 *
		 * @param player_id id of the player
		 * @param direction north, east, south or west
		 * @return          false if he is unknown or there is no exit this way
		 */
		bool move(long long player_id, const std::string& direction);

		//########################### ITEMS ############################
		/**
		 * Lists the items that a player carries.
		 *
		 * @param player_id id of the player
		 * @return          the ids of the definitions of his items, in alphabetical order (nothing if he is unknown)
		 */
		std::optional<std::vector<std::string>> showInventories(long long player_id) const;

		/**
		 * Takes an item from the floor of the room of a player. The first time it is taken, the item is planned to come back.
		 *
		 * @param player_id id of the player
		 * @param target    designates the item, as for Inventory::findItem
		 * @return          Moved and the id of the item; NotFound if there is no such item or it cannot be taken; DestinationFull if the inventory is full
		 */
		TransferResult takeItem(long long player_id, const std::string& target);

		/**
		 * Drops an item of a player on the floor of his room.
		 *
		 * @param player_id id of the player
		 * @param target    designates the item, as for Inventory::findItem
		 * @return          Moved and the id of the item, NotFound if the player does not have it
		 */
		TransferResult dropItem(long long player_id, const std::string& target);

		/**
		 * Uses an item of the inventory of a player: the item is consumed and its effect is applied.
		 *
		 * @param player_id id of the player
		 * @param target    designates the item, as for Inventory::findItem
		 * @return          USED with the id of the item and the hit points he got; NOT_FOUND if he does not have it;
		 *                  NOT_USABLE if it has no effect (it is kept); UNKNOWN_PLAYER
		 */
		UseResult useItem(long long player_id, const std::string& target);

		/**
		 * Makes a player talk to an npc of his room.
		 *
		 * @param player_id id of the player
		 * @param target    designates the npc, as for Room::findNpc
		 * @return          a line the npc says (nothing if there is no such npc or the player is unknown)
		 */
		std::optional<std::string> talk(long long player_id, const std::string& target) const;

		//########################### COMBAT ###########################
		/**
		 * Attacks an enemy: starts a fight, joins the fight of this enemy, or plays an attack in the fight in progress.
		 *
		 * @param player_id id of the player
		 * @param target    the enemy to attack; empty (or the same enemy) during a fight
		 * @return          what happened, or why nothing did
		 */
		CombatReport attack(long long player_id, const std::string& target);

		/**
		 * Plays a defense in the fight in progress.
		 *
		 * @param player_id id of the player
		 * @return          what happened, or why nothing did
		 */
		CombatReport defend(long long player_id);

		/**
		 * Tries to run away from the fight in progress.
		 *
		 * @param player_id id of the player
		 * @return          what happened, or why nothing did
		 */
		CombatReport flee(long long player_id);

		/** Tells whether a player is in a fight. */
		bool inCombat(long long player_id) const;

		/** Describes the fight of a player for STATUS (nothing if he is not in a fight). */
		std::optional<CombatView> combatView(long long player_id) const;

		//############################ GROUP ###########################
		/**
		 * Creates a group; the player is its leader.
		 *
		 * @param player_id id of the player
		 * @return          DONE and the id of the group, or ALREADY_IN_GROUP
		 */
		GroupReply groupCreate(long long player_id);

		/**
		 * Invites a player in the group of the inviter (he must be the leader). The invited player is told with an EVT GROUP INVITE.
		 *
		 * @param player_id id of the inviter
		 * @param target    name of the invited player
		 * @return          DONE; NOT_IN_GROUP, NOT_LEADER, SELF, ALREADY_IN_GROUP, GROUP_FULL, or unknown_player if nobody has this name
		 */
		GroupReply groupInvite(long long player_id, const std::string& target);

		/**
		 * Joins the group of a player who invited. The members are told with an EVT GROUP JOIN.
		 *
		 * @param player_id id of the player who joins
		 * @param target    name of a member of the group
		 * @return          DONE and the id of the group; ALREADY_IN_GROUP, NO_INVITATION, GROUP_FULL, or unknown_player if nobody has this name
		 */
		GroupReply groupJoin(long long player_id, const std::string& target);

		/**
		 * Leaves the group. The members who stay are told with an EVT GROUP LEAVE.
		 *
		 * @param player_id id of the player
		 * @return          DONE, or NOT_IN_GROUP
		 */
		GroupReply groupLeave(long long player_id);

		//############################ QUESTS ##########################
		/**
		 * Talks to an npc about quests: gives him the result of a quest in progress, or takes the quest he offers.
		 *
		 * @param player_id id of the player
		 * @param target    designates the npc, as for Room::findNpc
		 * @return          STARTED, IN_PROGRESS (the objective is not reached), COMPLETED, NONE (nothing to give or take),
		 *                  NPC_NOT_FOUND, INVENTORY_FULL (the reward does not fit), or UNKNOWN_PLAYER; with the quest concerned
		 */
		QuestReply quest(long long player_id, const std::string& target);

		/**
		 * Lists the quests of a player, in progress and finished.
		 *
		 * @param player_id id of the player
		 * @return          the quests (nothing if he is unknown)
		 */
		std::optional<std::vector<QuestView>> quests(long long player_id) const;

		//############################ CHAT ############################
		/**
		 * Sends a chat message to the players of a scope; the sender receives it too.
		 *
		 * @param player_id id of the sender
		 * @param scope     who receives it: everybody, the players of his room, or the players of his group
		 * @param text      the message, already checked with isValidChatMessage
		 * @return          SENT, NOT_IN_GROUP for the group scope if he has no group, or UNKNOWN_PLAYER
		 */
		ChatStatus chat(long long player_id, ChatScope scope, const std::string& text);

		//############################ TIME ############################
		/**
		 * Lets the time pass: plays the turns of the players who were too slow, brings back what is due, and applies the effects of the rooms.
		 * It is called at each turn of the main loop.
		 *
		 * @param now the current time
		 */
		void tick(std::chrono::steady_clock::time_point now);

		/**
		 * Takes the events that were queued, to deliver them.
		 *
		 * @return the events since the last call
		 */
		std::vector<Event> takeEvents();
};

#endif
