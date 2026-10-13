#ifndef LOOKUP_HPP
# define LOOKUP_HPP

# include <algorithm>
# include <cctype>
# include <sstream>
# include <string>
# include <unordered_map>
# include <vector>
# include <utility>

// How a player designates an NPC or an item (TALK guard, TAKE herbs, DROP healing herbs).
// The functions work on any object that has id(), name() and definition().id(): a Npc, an Item.
// Everything is in this header because the functions are templates: a room uses them to find
// an object on the floor, the game to find an object in the inventory of a player.
namespace lookup {

	inline std::string lowerCase(const std::string& text) {
		std::string lower = text;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
		return lower;
	}

	// True if the query (already in lower case) designates the object: its instance id, the id of its
	// definition (with or without its prefix), its name or one word of its name
	template <typename T>
	bool designates(const T& object, const std::string& query) {
		const std::string definition_id = lowerCase(object.definition().id());
		const size_t dot = definition_id.find('.');
		const std::string short_id = (dot == std::string::npos) ? definition_id : definition_id.substr(dot + 1);	// "guard"
		const std::string name = lowerCase(object.name());

		if (query == lowerCase(object.id()) || query == definition_id || query == short_id || query == name)
			return true;

		std::istringstream words(name);
		std::string word;
		while (words >> word) {
			if (word.back() == ',')
				word.pop_back();
			if (word == query)
				return true;
		}
		return false;
	}

	// The object of the map (by instance id) that the query designates, any case, spaces around ignored.
	// If several match, the first by instance id, so the answer is always the same. nullptr if none.
	template <typename T>
	const T* findIn(const std::unordered_map<std::string, T>& objects, const std::string& query) {
		const size_t first = query.find_first_not_of(' ');
		if (first == std::string::npos)
			return nullptr;
		const std::string wanted = lowerCase(query.substr(first, query.find_last_not_of(' ') - first + 1));

		std::vector<std::string> ids;
		for (const auto& [id, object] : objects)
			ids.push_back(id);
		std::sort(ids.begin(), ids.end());

		for (const std::string& id : ids) {
			const T& object = objects.at(id);
			if (designates(object, wanted))
				return &object;
		}
		return nullptr;
	}

	template <typename T>
	T* findIn(std::unordered_map<std::string, T>& objects, const std::string& query) {
		return const_cast<T*>(findIn(std::as_const(objects), query));
	}

}

#endif
