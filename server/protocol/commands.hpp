#ifndef COMMANDS_HPP
# define COMMANDS_HPP

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../game/game.hpp"
# include "../network/server.hpp"
# include "serializer.hpp"

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Defines the commands of the protocol: what each one does with the game.
 *
 * @param server receives the commands
 * @param game   the game they act on
 */
void registerCommands(Server& server, Game& game);

#endif
