#ifndef LOADENV_HPP
# define LOADENV_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Reads a file of lines KEY=VALUE and puts them in the environment of the process.
 *
 * Empty lines and lines that begin with # are ignored, the spaces around the key and the value
 * are removed, and so is a pair of quotes around the value. A line with no key, or with a key
 * that has characters other than letters, digits and underscores, is ignored.
 * A variable that already exists is not changed: the command line wins over the file.
 *
 * @param filename path of the file
 * @return         false if the file cannot be opened
 */
bool loadEnv(const std::string& filename = ".env");

/**
 * Loads the file of the settings of the project (ENV_FILE), if there is one.
 *
 * The programs run from server/ or from clients/cli/, so the file is looked for in the current folder,
 * then in the parent folder, then in the grandparent folder. The first one that exists is read, as by loadEnv().
 *
 * @return true if a file was found and read
 */
bool loadDefaultEnv();

/**
 * Reads a variable of the environment, safely.
 *
 * @param name     name of the variable
 * @param fallback what is returned if the variable does not exist or is empty
 * @return         the value of the variable, or the fallback
 */
std::string getEnv(const char* name, const std::string& fallback = "");

/**
 * Reads a variable of the environment as a number, safely.
 *
 * @param name     name of the variable
 * @param fallback what is returned if the variable does not exist, is not a number (a text, or a number
 *                 followed by other characters), or is out of the range
 * @param min      lowest value accepted
 * @param max      highest value accepted
 * @param rejected set to true if the variable exists but was refused (not a number, or out of the range); left alone otherwise
 * @return         the number, or the fallback
 */
long getEnvInt(const char* name, long fallback, long min, long max, bool* rejected = nullptr);

#endif
