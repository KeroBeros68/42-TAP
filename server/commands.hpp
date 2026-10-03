#ifndef COMMANDS_HPP
# define COMMANDS_HPP

# include "server.hpp"
# include "game.hpp"

// Registers every protocol command on the server.
// The commands reach the game state through the Game reference they capture.
void registerCommands(Server& server, Game& game);

#endif
