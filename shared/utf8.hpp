#ifndef UTF8_HPP
# define UTF8_HPP

# include <string>

bool validateUTF8(const std::string& text, bool allow_tab = false);

std::string jsonEscape(const std::string& text);

#endif
