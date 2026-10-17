#ifndef ERRORS_HPP
# define ERRORS_HPP

# include <string>

# include "defines.hpp"

// All the errors of the protocol: X(name, code). To add an error, add a line here.
# define TAP_ERRORS(X) \
	X(BAD_REQUEST,				400)	/* not in the RFC: unknown command / malformed message */ \
	X(NAME_IN_USE,				201) \
	X(NO_EXIT,					301) \
	X(NOT_IN_GROUP,				401) \
	X(ALREADY_IN_GROUP,			402) \
	X(INVENTORY_FULL,			403)	/* not in the RFC: TAKE with a full inventory */ \
	X(ITEM_NOT_FOUND,			404) \
	X(ITEM_NOT_IN_INVENTORY,	404) \
	X(NPC_NOT_FOUND,			404) \
	X(NPC_NOT_HOSTILE,			405) \
	X(NO_QUEST_AVAILABLE,		406) \
	X(IN_COMBAT,				409)	/* not in the RFC: MOVE, TAKE or TALK during a fight, or another target for ATTACK */ \
	X(NOT_IN_COMBAT,			410)	/* not in the RFC: DEFEND or FLEE with no fight */ \
	X(CONNECTION_FAILED,		900) \
	X(SEND_FAILED,				901)

enum class TapError {
# define X(name, code) name,
	TAP_ERRORS(X)
# undef X
};

inline int tapErrorCode(TapError e) {
	switch (e) {
# define X(name, code) case TapError::name: return code;
		TAP_ERRORS(X)
# undef X
	}
	return 0;
}

inline const char* tapErrorName(TapError e) {
	switch (e) {
# define X(name, code) case TapError::name: return #name;
		TAP_ERRORS(X)
# undef X
	}
	return "UNKNOWN";
}

// Full protocol line, e.g. "ERR 201 NAME_IN_USE" (without the trailing '\n')
inline std::string tapErrorLine(TapError e) {
	return std::string(RESPONSE_ERR) + " " + std::to_string(tapErrorCode(e)) + " " + tapErrorName(e);
}

#endif
