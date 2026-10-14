#ifndef SHARED_DEFINES_HPP
# define SHARED_DEFINES_HPP

// Values shared by the server and the clients (protocol + common system values)

# define SERVER_PORT		8080

# define INVALID_FD			-1
# define SYSCALL_ERROR		-1

# define LINE_END			'\n'
# define CARRIAGE_RETURN	'\r'
# define CMD_SEPARATOR		' '
# define MAX_LINE_LENGTH	1024
# define MAX_NAME_LENGTH	32

# define PLAYER_INVENTORY_CAPACITY	10
# define PLAYER_HIT				10
# define PLAYER_FLEE				10

// Chance to hit = hit / (hit + COMBAT_FLEE_WEIGHT * flee): 8 hit against 10 flee gives 80%
# define COMBAT_FLEE_WEIGHT		0.2

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
