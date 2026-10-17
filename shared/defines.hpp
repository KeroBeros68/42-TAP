#ifndef SHARED_DEFINES_HPP
# define SHARED_DEFINES_HPP

// Values shared by the server and the clients (protocol + common system values)

# define SERVER_PORT		8080

# define INVALID_FD			-1
# define SYSCALL_ERROR		-1
# define SIGNAL_NONE		-1		// nothing to read from a SignalFd

# define LINE_END			'\n'
# define CARRIAGE_RETURN	'\r'
# define CMD_SEPARATOR		' '
# define MAX_LINE_LENGTH	1024
# define MAX_NAME_LENGTH	32
# define POLL_TIMEOUT_MS	1000
# define RECV_BUFFER_SIZE	4096


// Words and fields of the protocol (the clients read them)
# define PROTOCOL_VERSION	"1"
# define RESPONSE_OK		"OK"
# define RESPONSE_ERR		"ERR"

# define STATUS_HEALTHY		"healthy"
# define STATUS_WOUNDED		"wounded"
# define STATUS_COMBAT		"combat"
# define STATUS_DEAD		"dead"

# define FIELD_ROOM			"room"
# define FIELD_ID			"id"
# define FIELD_NAME			"name"
# define FIELD_DESCRIPTION	"description"
# define FIELD_EXITS		"exits"
# define FIELD_PLAYERS		"players"
# define FIELD_ITEMS		"items"
# define FIELD_NPCS			"npcs"
# define FIELD_HP			"hp"
# define FIELD_MAX_HP		"max_hp"
# define FIELD_STATUS		"status"
# define FIELD_ATTACKER_HP	"attacker_hp"
# define FIELD_TARGET_HP	"target_hp"
# define FIELD_DAMAGE		"damage"

// Between the id of a definition and the counter of an instance: "npc.spider#13", "item.herbs#4"
# define INSTANCE_SEPARATOR	"#"

# define DIR_NORTH			"north"
# define DIR_EAST			"east"
# define DIR_SOUTH			"south"
# define DIR_WEST			"west"

# define CMD_CONNECT		"CONNECT"
# define CMD_LOOK 			"LOOK"
# define CMD_MOVE 			"MOVE"
# define CMD_CHAT 			"CHAT"
# define CMD_TAKE 			"TAKE"
# define CMD_DROP 			"DROP"
# define CMD_INVENTORY 		"INVENTORY"
# define CMD_TALK 			"TALK"
# define CMD_ATTACK 		"ATTACK"
# define CMD_STATUS 		"STATUS"
# define CMD_QUEST 			"QUEST"
# define CMD_QUESTS 		"QUESTS"
# define CMD_WHO 			"WHO"
# define CMD_GROUP 			"GROUP"
# define CMD_QUIT 			"QUIT"

#endif
