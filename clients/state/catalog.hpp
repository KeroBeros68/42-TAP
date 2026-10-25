/*
	Catalog is basically used to convert item IDs into names/descriptions,
	accordig to the loaded jsons, with a default fallback.
*/

#ifndef CATALOG_HPP
# define CATALOG_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <map>
# include <optional>
# include <string>

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

/**
 * Translates the opaque ids sent by the server into text fit for display.
 *
 * Only names and descriptions are kept: the catalog never carries game logic.
 * It is a bonus, never a requirement. An unknown id still gets a label, built
 * from the id itself, so the client always has something to show.
 */
class Catalog
{
	private:
		std::map<std::string, std::string>	_names;			// id, display name
		std::map<std::string, std::string>	_descriptions;	// id, description

		// id with its "#N" suffix removed, e.g. "item.herbs#3" -> "item.herbs"
		static std::string	stripInstance(const std::string& id);

		// label derived from the id, used when the catalog does not know it
		static std::string	fallback(const std::string& id);

	public:
		Catalog();

		// Merge the entries of one world file.
		// Returns false if the file is unreadable or malformed (not an error).
		bool				loadFile(const std::string& filePath);

		// Never empty: falls back to a label built from the id.
		std::string			name(const std::string& id) const;

		// Empty when the id is unknown or has no description.
		std::optional<std::string>	description(const std::string& id) const;
};

#endif