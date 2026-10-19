#ifndef UTF8_HPP
# define UTF8_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Checks that a text is valid UTF-8 and has no character that could hurt a log or a message.
 * The control characters, the C1 controls and the line and paragraph separators are refused.
 *
 * @param text      the text to check
 * @param allow_tab accept the tab character
 * @return          true if the text is acceptable
 */
bool validateUTF8(const std::string& text, bool allow_tab = false);

/**
 * Cuts a text without cutting a character in two.
 *
 * @param text the text
 * @param max  the length wanted, in bytes
 * @return     the largest length that is not above max and ends on the end of a character
 */
size_t utf8SafeLength(const std::string& text, size_t max);

/**
 * Escapes a text to put it between quotes in a JSON string.
 * A byte sequence that is not valid UTF-8 is replaced by \\ufffd.
 *
 * @param text the text to escape
 * @return     the escaped text
 */
std::string jsonEscape(const std::string& text);

#endif
