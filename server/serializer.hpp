#ifndef SERIALIZER_HPP
# define SERIALIZER_HPP

# include <string>

# include "../game/game.hpp"

// The JSON payload of the responses (LOOK, STATUS, ATTACK): the only place where they are written.
// The names of the fields are the ones of the RFC.
std::string serializeLook(const LookData& look);
std::string serializeStatus(const Stats& stats);
std::string serializeAttack(const CombatResult& combat);

#endif
