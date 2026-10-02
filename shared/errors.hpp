#ifndef ERRORS_HPP
# define ERRORS_HPP

# include <string>

enum class TapError {
	BAD_REQUEST,			// 400 (not in the RFC: unknown command / malformed message)
	NAME_IN_USE,			// 201
	NO_EXIT,				// 301
	NOT_IN_GROUP,			// 401
	ALREADY_IN_GROUP,		// 402
	ITEM_NOT_FOUND,			// 404
	ITEM_NOT_IN_INVENTORY,	// 404
	NPC_NOT_FOUND,			// 404
	NPC_NOT_HOSTILE,		// 405
	NO_QUEST_AVAILABLE,		// 406
	CONNECTION_FAILED,		// 900
	SEND_FAILED				// 901
};

inline int tapErrorCode(TapError e) {
	switch (e) {
		case TapError::BAD_REQUEST:				return 400;
		case TapError::NAME_IN_USE:				return 201;
		case TapError::NO_EXIT:					return 301;
		case TapError::NOT_IN_GROUP:			return 401;
		case TapError::ALREADY_IN_GROUP:		return 402;
		case TapError::ITEM_NOT_FOUND:			return 404;
		case TapError::ITEM_NOT_IN_INVENTORY:	return 404;
		case TapError::NPC_NOT_FOUND:			return 404;
		case TapError::NPC_NOT_HOSTILE:			return 405;
		case TapError::NO_QUEST_AVAILABLE:		return 406;
		case TapError::CONNECTION_FAILED:		return 900;
		case TapError::SEND_FAILED:				return 901;
	}
	return 0;
}

inline const char* tapErrorName(TapError e) {
	switch (e) {
		case TapError::BAD_REQUEST:				return "BAD_REQUEST";
		case TapError::NAME_IN_USE:				return "NAME_IN_USE";
		case TapError::NO_EXIT:					return "NO_EXIT";
		case TapError::NOT_IN_GROUP:			return "NOT_IN_GROUP";
		case TapError::ALREADY_IN_GROUP:		return "ALREADY_IN_GROUP";
		case TapError::ITEM_NOT_FOUND:			return "ITEM_NOT_FOUND";
		case TapError::ITEM_NOT_IN_INVENTORY:	return "ITEM_NOT_IN_INVENTORY";
		case TapError::NPC_NOT_FOUND:			return "NPC_NOT_FOUND";
		case TapError::NPC_NOT_HOSTILE:			return "NPC_NOT_HOSTILE";
		case TapError::NO_QUEST_AVAILABLE:		return "NO_QUEST_AVAILABLE";
		case TapError::CONNECTION_FAILED:		return "CONNECTION_FAILED";
		case TapError::SEND_FAILED:				return "SEND_FAILED";
	}
	return "UNKNOWN";
}

// Full protocol line, e.g. "ERR 201 NAME_IN_USE" (without the trailing '\n')
inline std::string tapErrorLine(TapError e) {
	return "ERR " + std::to_string(tapErrorCode(e)) + " " + tapErrorName(e);
}

#endif
