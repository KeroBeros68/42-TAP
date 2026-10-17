#ifndef LOOKUP_HPP
# define LOOKUP_HPP

# include <algorithm>
# include <cctype>
# include <sstream>
# include <string>
# include <unordered_map>
# include <vector>
# include <utility>

namespace lookup {

	inline std::string lowerCase(const std::string& text) {
		std::string lower = text;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
		return lower;
	}

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

	template <typename T>
	std::vector<std::string> sortedIds(const std::unordered_map<std::string, T>& objects) {
		std::vector<std::string> ids;
		ids.reserve(objects.size());
		for (const auto& [id, object] : objects)
			ids.push_back(id);
		std::sort(ids.begin(), ids.end());
		return ids;
	}

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

	template <typename T>
	T* findIn(std::unordered_map<std::string, T>& objects, const std::string& query) {
		return const_cast<T*>(findIn(std::as_const(objects), query));
	}

}

#endif
