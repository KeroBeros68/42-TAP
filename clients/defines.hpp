#ifndef DEFINES_HPP
# define DEFINES_HPP

# include "../shared/defines.hpp"

# define SERVER_IP			"127.0.0.1"

// Where the world files (items.json, npc.json...) are read from.
// The GUI is not started from the repository root, so the default path is
// relative to it; TAP_CATALOG_DIR overrides it.
# define ENV_CATALOG_DIR	"TAP_CATALOG_DIR"
// The build may know the absolute path of the repository (see CMakeLists.txt);
// otherwise we fall back to a path relative to the working directory.
# ifndef CATALOG_DEFAULT_DIR
#  define CATALOG_DEFAULT_DIR	"game/ressources/"
# endif

#endif
