#ifndef LOOKUP_HPP
# define LOOKUP_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <algorithm>
# include <cctype>
# include <sstream>
# include <string>
# include <unordered_map>
# include <utility>
# include <vector>

//##############################################################################
//#                                   LOOKUP                                   #
//##############################################################################

namespace lookup {

	/**
	 * Converts a text to lower case.
	 *
	 * @param text the text
	 * @return     the text in lower case
	 */
	inline std::string lowerCase(const std::string& text) {
		std::string lower = text;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
		return lower;
	}

	/**
	 * Tells whether a query designates an object.
	 * It matches the instance id, the id of the definition (with or without its prefix), the name, or one word of the name.
	 *
	 * @param object an npc or an item
	 * @param query  what the player typed, already in lower case
	 * @return       true if the object matches
	 */
	template <typename T>
	bool designates(const T& object, const std::string& query) {
		const std::string definition_id = lowerCase(object.definition().id());
		const size_t dot = definition_id.find('.');
		const std::string short_id = (dot == std::string::npos) ? definition_id : definition_id.substr(dot + 1);
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

	/**
	 * Lists the ids of a map of objects in alphabetical order, so that the answers are always the same.
	 *
	 * @param objects the objects, by id
	 * @return        the ids, sorted
	 */
	template <typename T>
	std::vector<std::string> sortedIds(const std::unordered_map<std::string, T>& objects) {
		std::vector<std::string> ids;
		ids.reserve(objects.size());
		for (const auto& [id, object] : objects)
			ids.push_back(id);
		std::sort(ids.begin(), ids.end());
		return ids;
	}

	/**
	 * Lists the ids of the definitions of some objects, in alphabetical order (an id appears as many times as there are objects of it).
	 *
	 * @param objects the objects, by instance id
	 * @return        the ids of their definitions
	 */
	template <typename T>
	std::vector<std::string> definitionIds(const std::unordered_map<std::string, T>& objects) {
		std::vector<std::string> ids;
		ids.reserve(objects.size());
		for (const auto& [id, object] : objects)
			ids.push_back(object.definition().id());
		std::sort(ids.begin(), ids.end());
		return ids;
	}

	/**
	 * Finds the object that a query designates, in any case and without the spaces around it.
	 *
	 * @param objects the objects, by instance id
	 * @param query   what the player typed
	 * @return        the object (the first by id if several match), nullptr if there is none
	 */
	template <typename T>
	const T* findIn(const std::unordered_map<std::string, T>& objects, const std::string& query) {
		const size_t first = query.find_first_not_of(' ');
		if (first == std::string::npos)
			return nullptr;
		const std::string wanted = lowerCase(query.substr(first, query.find_last_not_of(' ') - first + 1));

		for (const std::string& id : sortedIds(objects)) {
			const T& object = objects.at(id);
			if (designates(object, wanted))
				return &object;
		}
		return nullptr;
	}

	/**
	 * Same as the const version, but gives an object that can be modified.
	 *
	 * @param objects the objects, by instance id
	 * @param query   what the player typed
	 * @return        the object (the first by id if several match), nullptr if there is none
	 */
	template <typename T>
	T* findIn(std::unordered_map<std::string, T>& objects, const std::string& query) {
		return const_cast<T*>(findIn(std::as_const(objects), query));
	}

}

#endif
