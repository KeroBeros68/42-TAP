#ifndef LOGGER_HPP
# define LOGGER_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <atomic>
# include <chrono>
# include <cstdio>
# include <cstdlib>
# include <ctime>
# include <fstream>
# include <iostream>
# include <mutex>
# include <string>
# include <type_traits>
# include <unistd.h>
# include <utility>
# include <vector>

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Colors -------------------------------------------------------------------
# define COLOR_RED		"\033[31m"	// terminal color of the errors
# define COLOR_YELLOW	"\033[33m"	// terminal color of the warnings
# define COLOR_RESET	"\033[0m"	// back to the normal color

// -- Log format ---------------------------------------------------------------
# define LOG_TIMESTAMP_FORMAT	"%Y-%m-%dT%H:%M:%S"	// strftime format of the date
# define LOG_MS_FORMAT			"%s.%03d"			// date followed by the milliseconds
# define LOG_OFFSET_FORMAT		"%z"				// strftime format of the offset to UTC
# define LOG_UTC_OFFSET			"+00:00"			// offset written when the local one is not known
# define LOG_MS_PER_SECOND		1000				// milliseconds in a second
# define LOG_DATE_SIZE			32					// size of the buffer of the date
# define LOG_STAMP_SIZE			48					// size of the buffer of the date with its milliseconds
# define LOG_OFFSET_SIZE		16					// size of the buffer of the offset
# define LOG_RAW_OFFSET_LENGTH	5					// length of an offset like +0200
# define LOG_OFFSET_HOURS_LENGTH	3				// length of the hours of an offset, like +02
# define LOG_NO_COLOR_ENV		"NO_COLOR"			// environment variable that turns the colors off
# define LOG_KEY_TS				"ts"				// key of the date in a log line
# define LOG_KEY_LEVEL			"level"				// key of the level in a log line
# define LOG_KEY_MSG			"msg"				// key of the message in a log line
# define LOG_KEY_SRC			"src"				// key of the file and line in a log line

// -- Log macro ----------------------------------------------------------------
# define LOG_AT(level, msg, ...) /* logs only if the level is enabled */ \
	do { \
		if (Logger::instance().enabled(level)) \
			Logger::instance().log(level, __FILE__, __LINE__, msg __VA_OPT__(, LogFields{__VA_ARGS__})); \
	} while (0)

// -- Log levels ---------------------------------------------------------------
# define LOG_DEBUG(msg, ...) LOG_AT(LogLevel::Debug, msg __VA_OPT__(,) __VA_ARGS__)	// log of level debug
# define LOG_INFO(msg, ...)  LOG_AT(LogLevel::Info, msg __VA_OPT__(,) __VA_ARGS__)	// log of level info
# define LOG_WARN(msg, ...)  LOG_AT(LogLevel::Warn, msg __VA_OPT__(,) __VA_ARGS__)	// log of level warn
# define LOG_ERROR(msg, ...) LOG_AT(LogLevel::Error, msg __VA_OPT__(,) __VA_ARGS__)	// log of level error

//##############################################################################
//#                                   ENUMS                                    #
//##############################################################################

enum class LogLevel { Debug, Info, Warn, Error };

//##############################################################################
//#                                 LOG VALUE                                  #
//##############################################################################

class LogValue {
	private:
		//######################### VARIABLES ##########################
		std::string	_json;

	public:
		//######################## CONSTRUCTORS ########################
		/**
		 * Wraps a C string as a JSON string (a null pointer gives null).
		 *
		 * @param text the text, or nullptr
		 */
		LogValue(const char* text);

		/** Wraps a string as a JSON string. */
		LogValue(const std::string& text);

		/** Wraps a boolean as true or false. */
		LogValue(bool value);

		/** Wraps any number (but a bool) as a JSON number. */
		template <typename T>
			requires (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
		LogValue(T number) : _json(std::to_string(number)) {}

		//########################## GETTERS ###########################
		/** Returns the JSON text of the value. */
		const std::string&	json() const;
};

//##############################################################################
//#                                   TYPES                                    #
//##############################################################################

typedef std::vector<std::pair<std::string, LogValue> >	LogFields;

//##############################################################################
//#                                   LOGGER                                   #
//##############################################################################

class Logger {
	private:
		//######################### VARIABLES ##########################
		std::ofstream			_file;
		std::ostream*			_out;
		std::mutex				_mutex;
		std::atomic<LogLevel>	_min_level;
		std::string				_formattedUtcOffset;

		//######################## CONSTRUCTORS ########################
		/** Builds the logger (private: use Logger::instance()). */
		Logger();

		/** Copying is forbidden. */
		Logger(const Logger&);

		/** Copying is forbidden. */
		Logger& operator=(const Logger&);

		//########################## HELPERS ###########################
		/**
		 * Tells whether the logs may be written with colors on a stream.
		 *
		 * @param os the stream the logs are written to
		 * @return   true if it is a terminal and the NO_COLOR variable is not set
		 */
		static bool			useColor(const std::ostream& os);

		/**
		 * Writes the current date and time, in local time, with the milliseconds.
		 *
		 * @return the date, like 2026-10-14T09:42:31.859
		 */
		static std::string	timestamp();

		/**
		 * Writes the offset between the local time and UTC.
		 *
		 * @return the offset, like +02:00 (+00:00 if it is not known)
		 */
		static std::string	calculateOffset();

	public:
		//######################### SINGLETON ##########################
		/** Returns the only logger of the program. */
		static Logger&	instance();

		//########################## LOGGING ###########################
		/**
		 * Also writes the logs in a file, at its end.
		 *
		 * @param path the file to write
		 * @return     true if the file is open
		 */
		bool	openFile(const std::string& path);

		/**
		 * Sends the logs to a stream instead of the standard output and error.
		 *
		 * @param os the stream, which must outlive the logger
		 */
		void	setOutput(std::ostream& os);

		/**
		 * Ignores the logs of a lower level.
		 *
		 * @param level the lowest level that is written
		 */
		void	setMinLevel(LogLevel level);

		/** Tells whether the logs of a level are written. */
		bool	enabled(LogLevel level) const;

		/**
		 * Writes one log line: a JSON object with the date, the level, the message and the fields.
		 * The warnings and errors also tell the file and the line of the call, and go to the standard error.
		 *
		 * @param level   level of the log
		 * @param file    source file of the call
		 * @param line    line of the call
		 * @param message what happened
		 * @param fields  values added to the line, by name
		 */
		void	log(LogLevel level, const char* file, int line, const std::string& message, const LogFields& fields = LogFields());
};

#endif
