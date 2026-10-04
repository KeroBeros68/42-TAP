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
