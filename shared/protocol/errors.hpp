#ifndef ERRORS_HPP
# define ERRORS_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../defines.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Errors -------------------------------------------------------------------
# define TAP_ERRORS(X) /* every error of the protocol: X(name, code) */ \
	X(BAD_REQUEST,				400) /* unknown command or malformed message (not in the RFC) */ \
	X(NAME_IN_USE,				201) /* another player already has this name */ \
	X(NO_EXIT,					301) /* no exit in this direction */ \
	X(NOT_IN_GROUP,				401) /* the player is not in a group */ \
	X(ALREADY_IN_GROUP,			402) /* the player is already in a group */ \
	X(INVENTORY_FULL,			403) /* TAKE with a full inventory (not in the RFC) */ \
	X(ITEM_NOT_FOUND,			404) /* the item is not in the room */ \
	X(ITEM_NOT_IN_INVENTORY,	404) /* the item is not in the inventory */ \
	X(NPC_NOT_FOUND,			404) /* the npc is not in the room */ \
	X(NPC_NOT_HOSTILE,			405) /* ATTACK on an npc that is not an enemy */ \
	X(NO_QUEST_AVAILABLE,		406) /* the npc has no quest to give */ \
	X(ITEM_NOT_USABLE,			407) /* USE on an item that has no effect (not in the RFC) */ \
	X(IN_COMBAT,				409) /* MOVE, TAKE or TALK during a fight, or another target for ATTACK (not in the RFC) */ \
	X(NOT_IN_COMBAT,			410) /* DEFEND or FLEE with no fight (not in the RFC) */ \
	X(NOT_YOUR_TURN,			411) /* it is the turn of someone else (not in the RFC) */ \
	X(RATE_LIMITED,				429) /* too many commands in a short time: slow down (not in the RFC) */ \
	X(INTERNAL_ERROR,			500) /* the server failed while handling the command (not in the RFC) */ \
	X(SERVER_BUSY,				503) /* the connection is refused: too many clients or connections too fast (not in the RFC) */ \
	X(CONNECTION_FAILED,		900) /* the connection could not be established */ \
	X(SEND_FAILED,				901) /* a message could not be sent */

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class TapError {
# define X(name, code) name,	// one enumerator per error
	TAP_ERRORS(X)
# undef X
};

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/** Returns the code of an error, like 404. */
inline int tapErrorCode(TapError e) {
	switch (e) {
# define X(name, code) case TapError::name: return code;	// the code of an error
		TAP_ERRORS(X)
# undef X
	}
	return 0;
}

/** Returns the name of an error, like "NPC_NOT_FOUND". */
inline const char* tapErrorName(TapError e) {
	switch (e) {
# define X(name, code) case TapError::name: return #name;	// the name of an error
		TAP_ERRORS(X)
# undef X
	}
	return "UNKNOWN";
}

/** Builds the line sent for an error, like "ERR 201 NAME_IN_USE" (without the line end). */
inline std::string tapErrorLine(TapError e) {
	return std::string(RESPONSE_ERR) + " " + std::to_string(tapErrorCode(e)) + " " + tapErrorName(e);
}

#endif
