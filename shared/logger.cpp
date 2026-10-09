#include "logger.hpp"

static std::string timestamp() {
	const auto now = std::chrono::system_clock::now();
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
	const std::time_t seconds = std::chrono::system_clock::to_time_t(now);

	std::tm utc;
	localtime_r(&seconds, &utc);

	char date[32];
	std::strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%S", &utc);

	char out[48];
	std::snprintf(out, sizeof(out), "%s.%03d", date, static_cast<int>(ms.count()));
	return out;
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

		size_t length = 0;
		unsigned code = 0;
		if (c >= 0xC2 && c <= 0xDF) { length = 2; code = c & 0x1F; }
		else if (c >= 0xE0 && c <= 0xEF) { length = 3; code = c & 0x0F; }
		else if (c >= 0xF0 && c <= 0xF4) { length = 4; code = c & 0x07; }

		bool valid = length != 0 && i + length <= text.size();
		for (size_t k = 1; valid && k < length; k++) {
			const unsigned char next = static_cast<unsigned char>(text[i + k]);
			if ((next & 0xC0) != 0x80)
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

		if (!valid) {
			unicode(0xFFFD);
			i++;
			continue;
		}
		if ((code >= 0x80 && code <= 0x9F) || code == 0x2028 || code == 0x2029)
			unicode(code);
		else
			out.append(text, i, length);
		i += length;
	}
	return out;
}

LogValue::LogValue(const char* text) : _json(text ? "\"" + jsonEscape(text) + "\"" : "null") {}

LogValue::LogValue(const std::string& text) : _json("\"" + jsonEscape(text) + "\"") {}

LogValue::LogValue(bool value) : _json(value ? "true" : "false") {}

const std::string& LogValue::json() const {
	return _json;
}

Logger::Logger() : _out(nullptr), _min_level(LogLevel::Debug) {}

Logger& Logger::instance() {
	static Logger logger;
	return logger;
}

void Logger::setOutput(std::ostream& os) {
	std::lock_guard<std::mutex> lock(_mutex);
	_out = &os;
}

void Logger::setMinLevel(LogLevel level) {
	_min_level = level;
}

bool Logger::enabled(LogLevel level) const {
	return level >= _min_level;
}

bool Logger::useColor(const std::ostream& os) {
	if (std::getenv("NO_COLOR"))
		return false;
	if (&os == &std::cout)
		return isatty(STDOUT_FILENO);
	if (&os == &std::cerr)
		return isatty(STDERR_FILENO);
	return false;
}

static const char* levelName(LogLevel level) {
	switch (level) {
		case LogLevel::Debug:	return "DEBUG";
		case LogLevel::Info:	return "INFO";
		case LogLevel::Warn:	return "WARN";
		case LogLevel::Error:	return "ERROR";
	}
	return "";
}

void Logger::log(LogLevel level, const char* file, int line, const std::string& message, const LogFields& fields) {
	if (!enabled(level))
		return;

	std::string json = "{\"ts\":\"" + timestamp() + "+02:00" + "\",\"level\":\"" + levelName(level) + "\",\"msg\":\"" + jsonEscape(message) + "\"";
	if (level >= LogLevel::Warn)
		json += ",\"src\":\"" + jsonEscape(std::string(file) + ":" + std::to_string(line)) + "\"";
	for (const auto& [key, value] : fields)
		json += ",\"" + jsonEscape(key) + "\":" + value.json();
	json += "}";

	std::lock_guard<std::mutex> lock(_mutex);

	std::ostream& os = _out ? *_out : (level >= LogLevel::Warn ? std::cerr : std::cout);
	const char* color = "";
	if (level == LogLevel::Error)
		color = COLOR_RED;
	else if (level == LogLevel::Warn)
		color = COLOR_YELLOW;

	if (*color && useColor(os))
		os << color << json << COLOR_RESET << std::endl;
	else
		os << json << std::endl;

	if (_file.is_open())
		_file << json << std::endl;
}

bool Logger::openFile(const std::string& path) {
	std::lock_guard<std::mutex> lock(_mutex);
	_file.open(path, std::ios::out | std::ios::app);
	return _file.is_open();
}
