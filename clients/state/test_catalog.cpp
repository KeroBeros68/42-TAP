#include "catalog.hpp"
#include <iostream>

static void show(const Catalog &catalog, const std::string &id)
{
	std::cout << "  " << id << " -> " << catalog.name(id);
	const std::optional<std::string> description = catalog.description(id);
	if (description)
		std::cout << "  [" << description->substr(0, 40) << "...]";
	std::cout << std::endl;
}

int main(void)
{
	Catalog catalog;

	std::cout << "Known ids (with and without instance suffix):" << std::endl;
	show(catalog, "item.herbs");
	show(catalog, "item.herbs#3");
	show(catalog, "npc.guard");
	show(catalog, "npc.orc_boss#12");
	show(catalog, "loc.square");
	show(catalog, "quest.cellar_pests");

	std::cout << "Unknown ids (fallback):" << std::endl;
	show(catalog, "item.healing_herbs#3");
	show(catalog, "npc.dragon");
	show(catalog, "loc.some_new_room");
	show(catalog, "herbs");
	show(catalog, "");

	std::cout << "Missing file: " << catalog.loadFile("does/not/exist.json") << std::endl;
	return 0;
}
