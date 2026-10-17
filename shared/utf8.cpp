#include "utf8.hpp"

namespace {

bool isContinuationByte(unsigned char c) {
	return (c & 0xC0) == 0x80;
}

struct Utf8Char {
	bool		valid;
	size_t		length;
	unsigned	code;
};

Utf8Char decodeUtf8(const std::string& text, size_t i) {
	const unsigned char first = static_cast<unsigned char>(text[i]);
	size_t length = 0;
	unsigned code = 0;
	if (first >= 0xC2 && first <= 0xDF) { length = 2; code = first & 0x1F; }
	else if (first >= 0xE0 && first <= 0xEF) { length = 3; code = first & 0x0F; }
	else if (first >= 0xF0 && first <= 0xF4) { length = 4; code = first & 0x07; }

	bool valid = length != 0 && i + length <= text.size();
	for (size_t k = 1; valid && k < length; k++) {
		const unsigned char next = static_cast<unsigned char>(text[i + k]);
		if (!isContinuationByte(next))
			valid = false;
		else
			code = (code << 6) | (next & 0x3F);
	}
	if (valid) {
		if (length == 3 && (code < 0x800 || (code >= 0xD800 && code <= 0xDFFF)))
			valid = false;
		else if (length == 4 && (code < 0x10000 || code > 0x10FFFF))
			valid = false;
	}

	if (!valid)
		return Utf8Char{false, 1, 0};
	return Utf8Char{true, length, code};
}

bool isForbidden(unsigned code) {
	return (code >= 0x80 && code <= 0x9F) || code == 0x2028 || code == 0x2029;
}

}

size_t utf8SafeLength(const std::string& text, size_t max) {
	if (text.size() <= max)
		return text.size();
	size_t cut = max;
	while (cut > 0 && isContinuationByte(static_cast<unsigned char>(text[cut])))
		cut--;
	return cut;
}

bool validateUTF8(const std::string& text, bool allow_tab) {
	size_t i = 0;
	while (i < text.size()) {
		const unsigned char c = static_cast<unsigned char>(text[i]);

		if (c < 0x80) {
			if ((c < 0x20 && !(allow_tab && c == '\t')) || c == 0x7F)
				return false;
			i++;
			continue;
		}

		const Utf8Char character = decodeUtf8(text, i);
		if (!character.valid || isForbidden(character.code))
			return false;
		i += character.length;
	}
	return true;
}

std::string jsonEscape(const std::string& text) {
	static const char* hex = "0123456789abcdef";
	std::string out;
	out.reserve(text.size());

	auto unicode = [&out](unsigned code) {
		out += "\\u";
		out += hex[(code >> 12) & 0xF];
		out += hex[(code >> 8) & 0xF];
		out += hex[(code >> 4) & 0xF];
		out += hex[code & 0xF];
	};

	size_t i = 0;
	while (i < text.size()) {
		const unsigned char c = static_cast<unsigned char>(text[i]);

		if (c < 0x80) {
			switch (c) {
				case '"':	out += "\\\""; break;
				case '\\':	out += "\\\\"; break;
				case '\n':	out += "\\n"; break;
				case '\r':	out += "\\r"; break;
				case '\t':	out += "\\t"; break;
				default:
					if (c < 0x20 || c == 0x7F)
						unicode(c);
					else
						out += static_cast<char>(c);
			}
			i++;
			continue;
		}

		const Utf8Char character = decodeUtf8(text, i);
		if (!character.valid)
			unicode(0xFFFD);
		else if (isForbidden(character.code))
			unicode(character.code);
		else
			out.append(text, i, character.length);
		i += character.length;
	}
	return out;
}
