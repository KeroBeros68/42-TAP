#ifndef CATALOG_HPP
# define CATALOG_HPP


# include <vector>
# include <map>
# include <string>

/*
This class is used to get the display name 
of a given id. example: item.herbs#1 -> Herbs
*/
class Catalog
{
    private:
        // itemID: Name
        std::map<std::string, std::string>  items;

    public:
        // Constructor
        Catalog();

        // Load the given files to add data to the catalog
        bool        loadFile(const std::string& filePath);

        // Returns the item's name, or a safe default if none is found
        std::string get(const std::string& itemID);

};

#endif // !CATALOG_HPP