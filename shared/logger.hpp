#ifndef LOGGER_HPP
# define LOGGER_HPP

# include <atomic>
# include <cstdlib>
# include <fstream>
# include <iostream>
# include <mutex>
# include <string>
# include <type_traits>
# include <utility>
# include <vector>
# include <unistd.h>
# include <chrono>
# include <cstdio>
# include <ctime>

# define COLOR_RED		"\033[31m"
# define COLOR_YELLOW	"\033[33m"
# define COLOR_RESET	"\033[0m"

enum class LogLevel { Debug, Info, Warn, Error };

std::string jsonEscape(const std::string& text);

class LogValue {
	private:
		std::string	_json;

	public:
		LogValue(const char* text);
		LogValue(const std::string& text);
		LogValue(bool value);
		template <typename T>
			requires (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
		LogValue(T number) : _json(std::to_string(number)) {}

		const std::string&	json() const;
};

typedef std::vector<std::pair<std::string, LogValue> >	LogFields;

# define LOG_AT(level, msg, ...) \
	do { \
		if (Logger::instance().enabled(level)) \
			Logger::instance().log(level, __FILE__, __LINE__, msg __VA_OPT__(, LogFields{__VA_ARGS__})); \
	} while (0)

# define LOG_DEBUG(msg, ...) LOG_AT(LogLevel::Debug, msg __VA_OPT__(,) __VA_ARGS__)
# define LOG_INFO(msg, ...)  LOG_AT(LogLevel::Info, msg __VA_OPT__(,) __VA_ARGS__)
# define LOG_WARN(msg, ...)  LOG_AT(LogLevel::Warn, msg __VA_OPT__(,) __VA_ARGS__)
# define LOG_ERROR(msg, ...) LOG_AT(LogLevel::Error, msg __VA_OPT__(,) __VA_ARGS__)

class Logger {
	private:
		std::ofstream			_file;
		std::ostream*			_out;
		std::mutex				_mutex;
		std::atomic<LogLevel>	_min_level;

		Logger();
		Logger(const Logger&);
		Logger& operator=(const Logger&);

		static bool	useColor(const std::ostream& os);

	public:
		static Logger&	instance();

		bool	openFile(const std::string& path);
		void	setOutput(std::ostream& os);
		void	setMinLevel(LogLevel level);
		bool	enabled(LogLevel level) const;
		void	log(LogLevel level, const char* file, int line, const std::string& message, const LogFields& fields = LogFields());
};

#endif
