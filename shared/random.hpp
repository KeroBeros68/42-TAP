#ifndef SHARED_RANDOM_HPP
# define SHARED_RANDOM_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <cstddef>
# include <random>

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/** Returns the random generator of the thread, seeded once from the system. */
inline std::mt19937& randomGenerator() {
	thread_local std::mt19937 generator{std::random_device{}()};
	return generator;
}

/**
 * Draws an integer.
 *
 * @param min smallest value
 * @param max largest value
 * @return    a number in [min, max]
 */
inline int randomInt(int min, int max) {
	std::uniform_int_distribution<int> dist(min, max);
	return dist(randomGenerator());
}

/** Draws a number in [0, 1). */
inline double randomUnit() {
	std::uniform_real_distribution<double> dist(0.0, 1.0);
	return dist(randomGenerator());
}

/**
 * Draws the index of an element of a container.
 *
 * @param size number of elements, at least 1
 * @return     an index in [0, size)
 */
inline size_t randomIndex(size_t size) {
	std::uniform_int_distribution<size_t> dist(0, size - 1);
	return dist(randomGenerator());
}

#endif
