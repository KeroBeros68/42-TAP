#ifndef SHARED_DEFINES_HPP
# define SHARED_DEFINES_HPP

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Server -------------------------------------------------------------------
# define SERVER_PORT		8080	// TCP port the server listens on and the clients connect to

// -- Configuration ------------------------------------------------------------
# define ENV_FILE						".env"			// file of the settings, in the folder of the project
# define ENV_PORT						"SERVER_PORT"	// variable that overrides SERVER_PORT
# define ENV_IP							"SERVER_IP"		// variable that overrides the address of the server, for the clients
# define ENV_LOG_LEVEL					"LOG_LEVEL"		// variable for the lowest level of the logs: debug, info, warn or error
# define ENV_LOG_FILE					"LOG_FILE"		// variable for the file where the logs are also written (none if empty)
# define ENV_MAX_OUT_BUFFER				"MAX_OUT_BUFFER"				// variable that overrides MAX_OUT_BUFFER
# define ENV_MAX_READ_PER_TURN			"MAX_READ_PER_TURN"			// variable that overrides MAX_READ_PER_TURN
# define ENV_COMMAND_WINDOW_MS			"COMMAND_WINDOW_MS"			// variable that overrides COMMAND_WINDOW_MS
# define ENV_MAX_COMMANDS_PER_WINDOW	"MAX_COMMANDS_PER_WINDOW"	// variable that overrides MAX_COMMANDS_PER_WINDOW
# define ENV_MAX_FLOOD_WINDOWS			"MAX_FLOOD_WINDOWS"			// variable that overrides MAX_FLOOD_WINDOWS
# define ENV_MAX_SESSIONS				"MAX_SESSIONS"				// variable that overrides MAX_SESSIONS
# define ENV_MAX_SESSIONS_PER_IP		"MAX_SESSIONS_PER_IP"		// variable that overrides MAX_SESSIONS_PER_IP
# define ENV_CONNECT_WINDOW_MS			"CONNECT_WINDOW_MS"			// variable that overrides CONNECT_WINDOW_MS
# define ENV_MAX_CONNECTS_PER_WINDOW	"MAX_CONNECTS_PER_WINDOW"	// variable that overrides MAX_CONNECTS_PER_WINDOW
# define MIN_PORT						1				// lowest TCP port that can be used
# define MAX_PORT						65535			// highest TCP port

// -- Chat ---------------------------------------------------------------------
# define CHAT_SCOPE_GLOBAL	"GLOBAL"	// scope: every player of the server
# define CHAT_SCOPE_ROOM	"ROOM"		// scope: the players of the room of the sender
# define CHAT_SCOPE_GROUP	"GROUP"		// scope: the players of the group of the sender
# define MAX_CHAT_LENGTH	256			// longest chat message, in bytes

// -- System values ------------------------------------------------------------
# define INVALID_FD			-1	// value of a file descriptor that is not open
# define SYSCALL_ERROR		-1	// value returned by a system call that failed
# define SIGNAL_NONE		-1	// nothing to read from a SignalFd

// -- Line format and limits ---------------------------------------------------
# define LINE_END			'\n'	// ends every message of the protocol
# define CARRIAGE_RETURN	'\r'	// accepted before the line end, and ignored
# define CMD_SEPARATOR		' '		// separates a command from its arguments
# define MAX_LINE_LENGTH	1024	// longest line a client may send, in bytes
# define MAX_NAME_LENGTH	32		// longest player name, in bytes
# define POLL_TIMEOUT_MS	1000	// how long a poll waits before the loop runs again
# define RECV_BUFFER_SIZE	4096	// size of the buffer of each recv

// -- Protocol words -----------------------------------------------------------
# define PROTOCOL_VERSION	"1"		// version of the protocol, sent in the greeting
# define RESPONSE_OK		"OK"	// first word of a success response
# define RESPONSE_ERR		"ERR"	// first word of an error response
# define RESPONSE_EVT		"EVT"	// first word of an event

// -- Statuses -----------------------------------------------------------------
# define STATUS_HEALTHY		"healthy"	// status of a player who is not hurt
# define STATUS_WOUNDED		"wounded"	// status of a player who lost hit points
# define STATUS_COMBAT		"combat"	// status of a player who is in a fight
# define STATUS_DEAD		"dead"		// status of a player who has no hit points left

// -- Quest statuses -----------------------------------------------------------
# define QUEST_STATUS_ACTIVE		"active"	// status of a quest in progress
# define QUEST_STATUS_COMPLETED		"completed"	// status of a quest that is finished

// -- Response fields ----------------------------------------------------------
# define FIELD_ROOM			"room"						// the room described by LOOK
# define FIELD_ID			"id"						// id of the room
# define FIELD_NAME			"name"						// name of the room
# define FIELD_DESCRIPTION	"description"				// description of the room
# define FIELD_EXITS		"exits"						// exits of the room, by direction
# define FIELD_PLAYERS		"players"					// names of the players in the room
# define FIELD_ITEMS		"items"						// ids of the items on the floor
# define FIELD_NPCS			"npcs"						// ids of the npcs in the room
# define FIELD_HP			"hp"						// current hit points of the player
# define FIELD_MAX_HP		"max_hp"					// maximum hit points of the player
# define FIELD_STATUS		"status"					// status of the player (see STATUS_*)
# define FIELD_ATTACKER_HP	"attacker_hp"				// hit points of the player after the turns
# define FIELD_TARGET_HP	"target_hp"					// hit points of the enemy after the turns
# define FIELD_DAMAGE		"damage"					// damage dealt
# define FIELD_ROUND		"round"						// number of the round
# define FIELD_TURNS		"turns"						// turns played since the command
# define FIELD_ACTOR		"actor"						// who played the turn
# define FIELD_ACTION		"action"					// what the actor did (see ACTION_*)
# define FIELD_MISSED		"missed"					// the strike missed, or the escape failed
# define FIELD_QUEST_ID	"quest_id"					// id of a quest
# define FIELD_PROGRESS		"progress"					// progress of a quest, like "1/3"
# define FIELD_TEXT_DONE	"text_done"					// what the npc says when a quest is handed in
# define FIELD_REWARD		"reward"					// what a quest gives at the end
# define FIELD_HEAL			"heal"						// hit points given by a reward
# define FIELD_ESCAPED		"escaped"					// the player ran away from the fight
# define FIELD_ENEMY		"enemy"						// id of the enemy of the fight
# define FIELD_TARGET		"target"					// who suffered the turn
# define FIELD_TURN			"turn"						// who has to play
# define FIELD_YOUR_TURN	"your_turn"					// it is the turn of the player
# define FIELD_TURN_SECONDS_LEFT	"turn_seconds_left"	// seconds left before the turn is played for him

// -- Combat actions -----------------------------------------------------------
# define ACTION_ATTACK		"attack"	// the actor strikes
# define ACTION_DEFEND		"defend"	// the actor takes less damage until his next turn
# define ACTION_FLEE		"flee"		// the actor tries to run away

// -- Instance ids -------------------------------------------------------------
# define INSTANCE_SEPARATOR	"#"	// between the id of a definition and the counter of an instance (npc.spider#13)

// -- Directions ---------------------------------------------------------------
# define DIR_NORTH			"north"	// exit to the north
# define DIR_EAST			"east"	// exit to the east
# define DIR_SOUTH			"south"	// exit to the south
# define DIR_WEST			"west"	// exit to the west

// -- Commands -----------------------------------------------------------------
# define CMD_CONNECT		"CONNECT"	// choose a name and enter the game
# define CMD_LOOK 			"LOOK"		// describe the room
# define CMD_MOVE 			"MOVE"		// go through an exit
# define CMD_CHAT 			"CHAT"		// talk to the other players
# define CMD_TAKE 			"TAKE"		// take an item from the floor
# define CMD_DROP 			"DROP"		// drop an item on the floor
# define CMD_USE			"USE"		// use an item of the inventory
# define CMD_INVENTORY 		"INVENTORY"	// list the items of the player
# define CMD_TALK 			"TALK"		// talk to an npc
# define CMD_ATTACK 		"ATTACK"	// start a fight, or play an attack
# define CMD_STATUS 		"STATUS"	// hit points and state of the fight
# define CMD_QUEST 			"QUEST"		// ask a quest to an npc
# define CMD_QUESTS 		"QUESTS"	// list the quests of the player
# define CMD_WHO 			"WHO"		// list the players
# define CMD_GROUP 			"GROUP"		// manage a group of players
# define CMD_QUIT 			"QUIT"		// leave the game
# define CMD_DEFEND			"DEFEND"	// play a defense in a fight
# define CMD_FLEE			"FLEE"		// try to run away from a fight

// -- Group sub-commands -------------------------------------------------------
# define GROUP_CREATE	"CREATE"	// GROUP CREATE: creates a group
# define GROUP_INVITE	"INVITE"	// GROUP INVITE <player>: invites a player
# define GROUP_JOIN		"JOIN"		// GROUP JOIN <player>: joins the group of a player who invited
# define GROUP_LEAVE	"LEAVE"		// GROUP LEAVE: leaves the group

#endif
