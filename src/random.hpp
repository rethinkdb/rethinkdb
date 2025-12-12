// Copyright 2010-2016 RethinkDB, all rights reserved.

/// @file random.hpp
/// @brief Random number generation utilities for RethinkDB
///
/// This module provides thread-aware random number generation capabilities
/// using the Mersenne Twister algorithm (MT19937-64).
///
/// @note The rng_t class inherits from home_thread_mixin_debug_only_t to ensure
///       that random number generation operations occur on the thread where the
///       generator was created.
///
/// @defgroup RandomNumberGeneration Random Number Generation
/// Utilities for generating pseudo-random numbers in various formats
/// @{

#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <random>

#include "errors.hpp"
#include "threading.hpp"

/// @brief Thread-aware random number generator using Mersenne Twister algorithm
///
/// Generates pseudo-random numbers in various formats (int, uint64_t, double).
/// The generator maintains thread affinity and should only be used from the
/// thread on which it was created.
///
/// @example
/// @code
/// rng_t generator;
/// int dice_roll = generator.randint(6);  // Random int in [0, 6)
/// uint64_t random_id = generator.randuint64(1000000);  // 64-bit random
/// double fraction = generator.randdouble();  // Random double in [0, 1)
/// @endcode
class rng_t : public home_thread_mixin_debug_only_t {
public:
    /// @brief Constructs a random number generator seeded from OS entropy
    /// Default constructor initializes the Mersenne Twister with random seed
    rng_t();

    /// @brief Constructs a random number generator with explicit seed value
    /// @param seed The seed value for reproducible random sequences
    explicit rng_t(uint64_t seed);

    /// @brief Generates a uniform random integer in range [0, n)
    /// @param n The upper bound (exclusive) for the random number
    /// @return A random integer in [0, n)
    /// @note n must be > 0
    int randint(int n);

    /// @brief Generates a uniform random 64-bit unsigned integer in range [0, n)
    /// @param n The upper bound (exclusive) for the random number
    /// @return A random uint64_t in [0, n)
    /// @note n must be > 0
    uint64_t randuint64(uint64_t n);

    /// @brief Generates a uniform random size_t in range [0, n)
    /// @param n The upper bound (exclusive) for the random number
    /// @return A random size_t in [0, n)
    /// @note n must be > 0
    size_t randsize(size_t n);

    /// @brief Generates a uniform random double-precision float in [0, 1)
    /// @return A random double in [0, 1.0)
    double randdouble();

private:
    /// @brief The Mersenne Twister pseudo-random number generator engine
    std::mt19937_64 m_mt19937;

    DISABLE_COPYING(rng_t);
};

/// @brief Global random integer generator
/// Uses the default thread-local random number generator
/// @param n The upper bound (exclusive) for the random number
/// @return A random integer in [0, n)
int randint(int n);

/// @brief Global 64-bit random number generator
/// Uses the default thread-local random number generator
/// @param n The upper bound (exclusive) for the random number
/// @return A random uint64_t in [0, n)
uint64_t randuint64(uint64_t n);

/// @brief Global size_t random number generator
/// Uses the default thread-local random number generator
/// @param n The upper bound (exclusive) for the random number
/// @return A random size_t in [0, n)
size_t randsize(size_t n);

/// @brief Global double-precision random number generator
/// Uses the default thread-local random number generator
/// @return A random double in [0, 1.0)
double randdouble();

/// @}

#endif // RANDOM_HPP_
