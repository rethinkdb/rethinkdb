/**
 * @file time.hpp
 * @brief Time measurement and manipulation utilities.
 *
 * Provides functions for measuring time in microseconds and ticks,
 * obtaining system time, and manipulating timespec structures.
 */

#ifndef TIME_HPP_
#define TIME_HPP_

#include <stdint.h>
#include <time.h>

/**
 * @defgroup TimeUtilities Time Measurement and Manipulation
 * @brief Functions for time operations and conversions
 */

/**
 * @ingroup TimeUtilities
 * @brief Unsigned 64-bit integer type for microsecond-precision time.
 *
 * Represents elapsed time in microseconds since an unspecified epoch.
 */
typedef uint64_t microtime_t;

/**
 * @ingroup TimeUtilities
 * @brief Get the current time in microseconds.
 *
 * @return The current time as microseconds since an unspecified epoch.
 *
 * Example:
 * @code
 * microtime_t start = current_microtime();
 * // ... do work ...
 * microtime_t elapsed = current_microtime() - start;  // In microseconds
 * @endcode
 */
microtime_t current_microtime();

/**
 * @ingroup TimeUtilities
 * @brief Get the current monotonic clock time.
 *
 * Returns a timespec structure with the current monotonic clock value.
 * Monotonic time never goes backwards and is unaffected by system clock adjustments.
 *
 * @return Current time from the monotonic clock.
 *
 * Example:
 * @code
 * timespec start = clock_monotonic();
 * // ... do work ...
 * timespec end = clock_monotonic();
 * @endcode
 */
timespec clock_monotonic();

/**
 * @ingroup TimeUtilities
 * @brief Get the current real-time clock.
 *
 * Returns a timespec structure with the current real-time (wall clock) value.
 * Real-time can jump backwards if the system clock is adjusted.
 *
 * @return Current wall-clock time.
 *
 * Example:
 * @code
 * timespec now = clock_realtime();
 * // Use for getting actual date/time
 * @endcode
 */
timespec clock_realtime();

/**
 * @ingroup TimeUtilities
 * @brief Add nanoseconds to a timespec structure.
 *
 * Modifies the provided timespec by adding the specified nanoseconds.
 * Handles overflow of the nanosecond field into seconds.
 *
 * @param ts Pointer to the timespec to modify.
 * @param nanoseconds The number of nanoseconds to add.
 *
 * Example:
 * @code
 * timespec ts = clock_monotonic();
 * add_to_timespec(&ts, 500000000);  // Add 0.5 seconds
 * @endcode
 */
void add_to_timespec(timespec *ts, int32_t nanoseconds);

/**
 * @ingroup TimeUtilities
 * @brief Subtract two timespecs.
 *
 * Computes t1 - t2, returning the difference as a timespec.
 *
 * @param t1 The minuend (first operand).
 * @param t2 The subtrahend (second operand).
 * @return The result of t1 - t2.
 *
 * Example:
 * @code
 * timespec start = clock_monotonic();
 * // ... do work ...
 * timespec end = clock_monotonic();
 * timespec elapsed = subtract_timespecs(end, start);
 * @endcode
 */
timespec subtract_timespecs(const timespec &t1, const timespec &t2);

/**
 * @ingroup TimeUtilities
 * @brief Compare two timespecs for less-than relationship.
 *
 * @param t1 The first timespec.
 * @param t2 The second timespec.
 * @return true if t1 < t2, false otherwise.
 */
bool operator<(const struct timespec &t1, const struct timespec &t2);

/**
 * @ingroup TimeUtilities
 * @brief Compare two timespecs for greater-than relationship.
 *
 * @param t1 The first timespec.
 * @param t2 The second timespec.
 * @return true if t1 > t2, false otherwise.
 */
bool operator>(const struct timespec &t1, const struct timespec &t2);

/**
 * @ingroup TimeUtilities
 * @brief Compare two timespecs for less-than-or-equal relationship.
 *
 * @param t1 The first timespec.
 * @param t2 The second timespec.
 * @return true if t1 <= t2, false otherwise.
 */
bool operator<=(const struct timespec &t1, const struct timespec &t2);

/**
 * @ingroup TimeUtilities
 * @brief Compare two timespecs for greater-than-or-equal relationship.
 *
 * @param t1 The first timespec.
 * @param t2 The second timespec.
 * @return true if t1 >= t2, false otherwise.
 */
bool operator>=(const struct timespec &t1, const struct timespec &t2);

/**
 * @ingroup TimeUtilities
 * @brief Unsigned 64-bit integer type for tick-based time measurement.
 *
 * A "tick" is an arbitrary unit of time. Use ticks_to_secs() to convert
 * to seconds if needed.
 */
typedef uint64_t ticks_t;

/**
 * @ingroup TimeUtilities
 * @brief Convert seconds to ticks.
 *
 * @param secs The number of seconds.
 * @return The equivalent time in ticks.
 *
 * Example:
 * @code
 * ticks_t deadline = secs_to_ticks(60);  // 60 seconds from epoch
 * @endcode
 */
ticks_t secs_to_ticks(time_t secs);

/**
 * @ingroup TimeUtilities
 * @brief Get the current time in ticks.
 *
 * @return Current time as a tick count.
 *
 * Example:
 * @code
 * ticks_t start = get_ticks();
 * // ... do work ...
 * ticks_t elapsed = get_ticks() - start;
 * @endcode
 */
ticks_t get_ticks();

/**
 * @ingroup TimeUtilities
 * @brief Get the current time in seconds.
 *
 * @return The current UNIX time in seconds.
 *
 * Example:
 * @code
 * time_t now = get_secs();
 * @endcode
 */
time_t get_secs();

/**
 * @ingroup TimeUtilities
 * @brief Convert ticks to seconds.
 *
 * @param ticks The time in ticks.
 * @return The equivalent time in seconds as a double.
 *
 * Example:
 * @code
 * ticks_t delta = get_ticks() - start;
 * double seconds = ticks_to_secs(delta);
 * @endcode
 */
double ticks_to_secs(ticks_t ticks);


#endif  // TIME_HPP_
