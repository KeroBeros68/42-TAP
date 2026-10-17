#ifndef COMMANDS_HPP
# define COMMANDS_HPP

# include "../network/server.hpp"
# include "../../game/game.hpp"
# include "serializer.hpp"


void registerCommands(Server& server, Game& game);

#endif
