#ifndef PRESENCE_HPP
# define PRESENCE_HPP

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../entities/player.hpp"
# include "room.hpp"

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Logs ---------------------------------------------------------------------
# define LOG_HERE(here) /* fields that every log about a player in a room starts with */ \
	{"client", (here).player->id()}, {"player", (here).player->name()}, {"room", (here).room->id()}

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

template <typename P, typename R>
struct BasicPresence {
	P* player = nullptr;
	R* room = nullptr;

	/** Tells whether both the player and the room are known. */
	explicit operator bool() const { return player && room; }
};

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

typedef BasicPresence<Player, Room>				Presence;
typedef BasicPresence<const Player, const Room>	ConstPresence;

#endif
