#ifndef SHARED_RANDOM_HPP
# define SHARED_RANDOM_HPP

# include <cstddef>
# include <random>

inline std::mt19937& randomGenerator() {
	thread_local std::mt19937 generator{std::random_device{}()};
	return generator;
}

inline int randomInt(int min, int max) {
	std::uniform_int_distribution<int> dist(min, max);
	return dist(randomGenerator());
}

inline size_t randomIndex(size_t size) {
	std::uniform_int_distribution<size_t> dist(0, size - 1);
	return dist(randomGenerator());
}

#endif
