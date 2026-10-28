// Copyright 2010-2016 RethinkDB, all rights reserved.

/**
 * @file random.hpp
 * @brief Thread-local random number generation utilities.
 *
 * Provides a thread-local Mersenne Twister random number generator for
 * consistent pseudo-random number generation throughout RethinkDB.
 */

#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <random>

#include "errors.hpp"

#include "threading.hpp"

/**
 * @defgroup RandomGeneration Random Number Generation
 * @brief Thread-local RNG utilities
 */

/**
 * @ingroup RandomGeneration
 * @brief Thread-local random number generator using Mersenne Twister.
 *
 * This class wraps std::mt19937_64 and uses the home_thread_mixin_debug_only_t
 * to verify that all RNG operations occur on the thread that created the object.
 *
 * Thread-safety: NOT thread-safe. Each thread should have its own rng_t instance.
 *
 * Example:
 * @code
 * rng_t rng;
 * int die_roll = rng.randint(6) + 1;  // Random number in [1, 6]
 * @endcode
 */
class rng_t : public home_thread_mixin_debug_only_t {
public:
    /**
     * @brief Construct a default-seeded RNG.
     *
     * Uses std::random_device for seeding (implementation-defined entropy source).
     */
    rng_t();

    /**
     * @brief Construct an RNG with a specific seed.
     *
     * @param seed The seed value for the Mersenne Twister.
     */
    explicit rng_t(uint64_t seed);

    /**
     * @brief Generate a uniform random integer in [0, n).
     *
     * @param n The upper bound (exclusive).
     * @return A random integer in the range [0, n).
     *
     * @code
     * int random_bit = rng.randint(2);  // 0 or 1
     * @endcode
     */
    int randint(int n);

    /**
     * @brief Generate a uniform random 64-bit unsigned integer in [0, n).
     *
     * @param n The upper bound (exclusive).
     * @return A random uint64_t in the range [0, n).
     */
    uint64_t randuint64(uint64_t n);

    /**
     * @brief Generate a uniform random size_t in [0, n).
     *
     * @param n The upper bound (exclusive).
     * @return A random size_t in the range [0, n).
     *
     * @code
     * size_t index = rng.randsize(vector.size());
     * @endcode
     */
    size_t randsize(size_t n);

    /**
     * @brief Generate a uniform random double in [0.0, 1.0).
     *
     * @return A random double in [0.0, 1.0).
     *
     * @code
     * double probability = rng.randdouble();  // For Monte Carlo simulations
     * @endcode
     */
    double randdouble();

private:
    std::mt19937_64 m_mt19937;

    DISABLE_COPYING(rng_t);
};

/**
 * @ingroup RandomGeneration
 * @brief Generate a thread-local random integer in [0, n).
 *
 * Uses a thread-local RNG instance (created on first use).
 *
 * @param n The upper bound (exclusive).
 * @return A random integer in [0, n).
 *
 * @code
 * int flip = randint(2);  // Heads (0) or tails (1)
 * @endcode
 */
int randint(int n);

/**
 * @ingroup RandomGeneration
 * @brief Generate a thread-local random 64-bit unsigned integer in [0, n).
 *
 * Uses a thread-local RNG instance (created on first use).
 *
 * @param n The upper bound (exclusive).
 * @return A random uint64_t in [0, n).
 */
uint64_t randuint64(uint64_t n);
size_t randsize(size_t n);

double randdouble();

#endif // RANDOM_HPP_
