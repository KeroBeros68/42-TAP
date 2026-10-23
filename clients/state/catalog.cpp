#include "catalog.hpp"

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "message.hpp"				// nlohmann::json
# include "../defines.hpp"				// ENV_CATALOG_DIR, CATALOG_DEFAULT_DIR
# include "../../shared/defines.hpp"	// INSTANCE_SEPARATOR
# include <cctype>
# include <cstdlib>					// std::getenv
# include <fstream>

using json = nlohmann::json;

//##############################################################################
//#                             INTERNAL FUNCTIONS                             #
//##############################################################################

namespace {

/**
 * Walks a world file and stores every object that carries a name.
 *
 * The files do not all have the same shape ("items", "npcs", "rooms"...), so
 * instead of knowing each of them we descend into the tree and keep whatever
 * looks like an entry. "start_room" and "respawn" have no name: they are
 * skipped without any special case.
 *
 * @param key   the key this node was found under: it is the id of an entry
 * @param node  the JSON node to inspect
 * @param names receives id -> name
 * @param descs receives id -> description, when there is one
 */
void walk(const std::string& key, const json& node,
	std::map<std::string, std::string>& names,
	std::map<std::string, std::string>& descs)
{
	if (!node.is_object())
		return;

	const json::const_iterator name = node.find("name");
	if (name != node.end() && name->is_string() && !key.empty())
	{
		names[key] = name->get<std::string>();

		const json::const_iterator description = node.find("description");
		if (description != node.end() && description->is_string())
			descs[key] = description->get<std::string>();
	}

	for (const auto& [childKey, child] : node.items())
		walk(childKey, child, names, descs);
}

// The helpers below build a readable label out of an id, so the client
// never shows "item.healing_herbs#3" to the player.

std::string withoutPrefix(const std::string& id)
{
	const size_t dot = id.find('.');
	return (dot == std::string::npos) ? id : id.substr(dot + 1);
}

std::string withoutInstance(const std::string& id)
{
	const size_t separator = id.rfind(INSTANCE_SEPARATOR);
	return (separator == std::string::npos) ? id : id.substr(0, separator);
}

std::string humanize(const std::string& id)
{
	std::string label = withoutPrefix(id);
	bool capitalize = true;
	for (char& c : label)
	{
		if (c == '_')
		{
			c = ' ';
			capitalize = true;
		}
		else if (capitalize && std::isalpha(static_cast<unsigned char>(c)))
		{
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
			capitalize = false;
		}
		else
			capitalize = false;
	}
	return label;
}

} // namespace

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

Catalog::Catalog()
{
	// The world files may live anywhere: the environment wins when it is set.
	// Without it, the default path is relative to the working directory.
	const char* environment = std::getenv(ENV_CATALOG_DIR);
	std::string directory = (environment != nullptr && *environment != '\0')
		? environment : CATALOG_DEFAULT_DIR;

	// A trailing separator keeps the concatenation below simple
	if (!directory.empty() && directory.back() != '/')
		directory += '/';

	loadFile(directory + "items.json");
	loadFile(directory + "npc.json");
	loadFile(directory + "quests.json");
	loadFile(directory + "room.json");
}

bool Catalog::loadFile(const std::string& filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open())
		return false;

	// allow_exceptions=false: a malformed file comes back discarded
	// instead of throwing, so a broken catalog never stops the client
	const json document = json::parse(file, nullptr, false);
	if (document.is_discarded())
		return false;

	walk("", document, this->_names, this->_descriptions);
	return true;
}

std::string Catalog::stripInstance(const std::string& id)
{
	return withoutInstance(id);
}

std::string Catalog::fallback(const std::string& id)
{
	return humanize(id);
}

std::string Catalog::name(const std::string& id) const
{
	const std::string key = stripInstance(id);
	const std::map<std::string, std::string>::const_iterator it = _names.find(key);
	if (it != _names.end())
		return it->second;
	return fallback(key);
}

std::optional<std::string> Catalog::description(const std::string& id) const
{
	const std::string key = stripInstance(id);
	const std::map<std::string, std::string>::const_iterator it = _descriptions.find(key);
	if (it != _descriptions.end())
		return it->second;
	return std::nullopt;
}