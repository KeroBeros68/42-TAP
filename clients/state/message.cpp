# include <cstdlib>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../../shared/defines.hpp"
# include "../../third_party/json.hpp"
# include "message.hpp"

//##############################################################################
//#                             INTERNAL FUNCTIONS                             #
//##############################################################################

namespace {

/**
 * Takes the first word out of text, and leaves the rest in rest.
 * Spaces around the word are skipped.
 */
std::string takeFirstWord(const std::string& text, std::string& rest)
{
	size_t begin = 0;
	while (begin < text.size() && text[begin] == CMD_SEPARATOR)
		++begin;
	size_t end = begin;
	while (end < text.size() && text[end] != CMD_SEPARATOR)
		++end;
	// Saving the first word
	const std::string token = text.substr(begin, end - begin);
	while (end < text.size() && text[end] == CMD_SEPARATOR)
		++end;
	// Saving the rest of the string inside the rest string reference
	rest = text.substr(end);
	return token;
}

/**
 * Reads a whole token as a number.
 * Returns false if the token is empty or holds anything else than digits.
 */
bool toNumber(const std::string& token, int& number)
{
	if (token.empty())
		return false;
	char* end = NULL;
	const long value = std::strtol(token.c_str(), &end, 10);
	if (end == NULL || *end != '\0')
		return false;
	number = static_cast<int>(value);
	return true;
}

} // namespace

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

ServerMessage parseLine(const std::string& line)
{
	ServerMessage message;

	// First word: the family of the message (OK, ERR, EVT, anything else)
	std::string rest;
	const std::string type = takeFirstWord(line, rest);

	if (type == RESPONSE_OK)
	{
		message.kind = ServerMessage::Kind::OK;
		message.data = rest;
	}
	else if (type == RESPONSE_ERR)
	{
		message.kind = ServerMessage::Kind::ERR;

		std::string afterCode;
		const std::string firstWord = takeFirstWord(rest, afterCode);

		// "ERR 404 ITEM_NOT_FOUND": the code is a number, the name follows
		// "ERR SOMETHING": no code, the first word is the name
		if (toNumber(firstWord, message.code))
			message.name = afterCode;
		else
		{
			message.code = 0;
			message.name = (afterCode.empty() ? firstWord : firstWord + " " + afterCode);
		}
	}
	else if (type == RESPONSE_EVT)
	{
		message.kind = ServerMessage::Kind::EVT;

		std::string afterCategory;
		message.category = takeFirstWord(rest, afterCategory);
		message.data = afterCategory;
	}
	else
	{
		// Not a protocol line: keep it whole, the journal will show it
		message.kind = ServerMessage::Kind::UNKNOWN;
		message.data = line;
	}
	return message;
}

std::optional<nlohmann::json> asJson(const std::string& data)
{
	// Only try to parse what looks like JSON
	size_t begin = 0;
	while (begin < data.size() && (data[begin] == ' ' || data[begin] == '\t'))
		++begin;
	// skip if not json
	if (begin == data.size() || (data[begin] != '{' && data[begin] != '['))
		return std::nullopt;

	try
	{
		// Try to parse it and return it
		return nlohmann::json::parse(data);
	}
	catch (const std::exception&)
	{
		// Malformed JSON: the caller logs it, the client keeps going
		return std::nullopt;
	}
}