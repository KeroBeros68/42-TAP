#ifndef SUCCESS_HPP
# define SUCCESS_HPP

# include <string>

// Initial greeting sent by the server on connection (RFC section 3.2)
# define SERVER_GREETING	"OK hello proto=1"

enum class TapOk {
	PLAIN,		// "OK"                  CHAT, GROUP INVITE, GROUP LEAVE
	DATA,		// "OK <payload>"        LOOK, INVENTORY, TALK, ATTACK, STATUS, QUEST, QUESTS
	CONNECTED,	// "OK connected"        CONNECT
	BYE,		// "OK bye"              QUIT
	ROOM,		// "OK room=<id>"        MOVE
	PLAYERS,	// "OK players=<count>"  WHO
	GROUP,		// "OK group=<id>"       GROUP CREATE, GROUP JOIN
	TAKEN,		// "OK taken=<item-id>"  TAKE
	DROPPED		// "OK dropped=<item-id>" DROP
};

inline const char* tapOkPrefix(TapOk r) {
	switch (r) {
		case TapOk::PLAIN:		return "";
		case TapOk::DATA:		return "";
		case TapOk::CONNECTED:	return "connected";
		case TapOk::BYE:		return "bye";
		case TapOk::ROOM:		return "room=";
		case TapOk::PLAYERS:	return "players=";
		case TapOk::GROUP:		return "group=";
		case TapOk::TAKEN:		return "taken=";
		case TapOk::DROPPED:	return "dropped=";
	}
	return "";
}

inline std::string tapOkLine(TapOk r, const std::string& payload = "") {
	std::string line = "OK";
	std::string body = std::string(tapOkPrefix(r)) + payload;
	if (!body.empty())
		line += " " + body;
	return line;
}

#endif
