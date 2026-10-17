#ifndef UTF8_HPP
# define UTF8_HPP

# include <string>

bool validateUTF8(const std::string& text, bool allow_tab = false);

// The longest length <= max that does not cut a character in two
size_t utf8SafeLength(const std::string& text, size_t max);

std::string jsonEscape(const std::string& text);

#endif
