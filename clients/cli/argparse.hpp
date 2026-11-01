#ifndef ARGPARSE_HPP
# define ARGPARSE_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  DEFINES                                   #
//##############################################################################

// -- Options ------------------------------------------------------------------
# define ARG_HOST		"--host"	// option: address of the server
# define ARG_HOST_SHORT	"-H"		// option: address of the server, short form
# define ARG_PORT		"--port"	// option: port of the server
# define ARG_PORT_SHORT	"-p"		// option: port of the server, short form
# define ARG_HELP		"--help"	// option: show the usage
# define ARG_HELP_SHORT	"-h"		// option: show the usage, short form

//##############################################################################
//#                                  ARG PARSE                                 #
//##############################################################################

struct ArgParse {
	//######################### VARIABLES ##########################
	std::string	hostname;
	int			port;
	bool		help = false;

	//######################## CONSTRUCTOR #########################
	/**
	 * Reads the arguments of the command line. The default values are the ones of the environment, so loadDefaultEnv()
	 * has to be called before. An option is written "--host 10.0.0.2" or "--host=10.0.0.2" ("-H 10.0.0.2"), and so is "--port"
	 * ("-p 4242"); the short forms take their value as the next argument.
	 * Nothing after --help (or -h) is read.
	 *
	 * @param argc number of arguments, as given to main
	 * @param argv the arguments, as given to main
	 * @throws std::invalid_argument for an unknown option, a missing value, a port that is not a number from MIN_PORT to
	 *         MAX_PORT, or a host that is not an IPv4 address; the message says what is wrong
	 */
	ArgParse(int argc, char** argv);

	//######################### FUNCTIONS ##########################
	/** Returns the text that explains the options. */
	static std::string usage();
};

#endif
