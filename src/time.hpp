/// @file time.hpp
/// @brief Time measurement and conversion utilities for RethinkDB
///
/// Provides functions for querying system clocks (monotonic and real-time),
/// converting between different time formats, and manipulating time values.
/// These utilities are essential for precise timing in a database system.
///
/// @defgroup TimeUtilities Time Measurement and Conversion
/// Functions for clock access, time conversions, and time arithmetic
/// @{

#ifndef TIME_HPP_
#define TIME_HPP_

#include <stdint.h>
#include <time.h>

/// @typedef microtime_t
/// @brief Time measurement in microseconds (uint64_t)
/// Represents a duration or timestamp with microsecond resolution
typedef uint64_t microtime_t;

/// @brief Gets the current time in microseconds
/// Returns the elapsed time since an arbitrary epoch
/// @return The current time in microseconds
/// @example
/// @code
/// microtime_t start = current_microtime();
/// // ... do some work ...
/// microtime_t end = current_microtime();
/// microtime_t elapsed = end - start;  // Duration in microseconds
/// @endcode
microtime_t current_microtime();

/// @brief Gets the current monotonic clock time
/// Returns time from a clock that always moves forward (not affected by system clock adjustments)
/// @return Current timespec structure with monotonic time
/// @see clock_realtime
/// @example
/// @code
/// timespec start = clock_monotonic();
/// // ... do work ...
/// timespec end = clock_monotonic();
/// // Compare using timespec comparison operators
/// @endcode
timespec clock_monotonic();

/// @brief Gets the current real-time (wall-clock) time
/// Returns the current system time that can be adjusted by system operators
/// @return Current timespec structure with real-time
/// @see clock_monotonic
/// @example
/// @code
/// timespec now = clock_realtime();
/// // Use with timers and scheduling
/// @endcode
timespec clock_realtime();

/// @brief Adds nanoseconds to a timespec structure
/// Modifies @p ts in-place by adding the specified nanoseconds
/// @param ts Pointer to the timespec to modify
/// @param nanoseconds The nanoseconds to add (can be negative)
/// @example
/// @code
/// timespec ts = clock_monotonic();
/// add_to_timespec(&ts, 500000000);  // Add 500 milliseconds
/// @endcode
void add_to_timespec(timespec *ts, int32_t nanoseconds);

/// @brief Subtracts one timespec from another
/// Computes the duration between two time points
/// @param t1 The later time
/// @param t2 The earlier time
/// @return The difference (t1 - t2) as a timespec
/// @example
/// @code
/// timespec start = clock_monotonic();
/// // ... work ...
/// timespec end = clock_monotonic();
/// timespec elapsed = subtract_timespecs(end, start);
/// @endcode
timespec subtract_timespecs(const timespec &t1, const timespec &t2);

/// @brief Less-than comparison for timespec structures
/// @param t1 First time value
/// @param t2 Second time value
/// @return true if t1 < t2, false otherwise
bool operator<(const struct timespec &t1, const struct timespec &t2);

/// @brief Greater-than comparison for timespec structures
/// @param t1 First time value
/// @param t2 Second time value
/// @return true if t1 > t2, false otherwise
bool operator>(const struct timespec &t1, const struct timespec &t2);

/// @brief Less-than-or-equal comparison for timespec structures
/// @param t1 First time value
/// @param t2 Second time value
/// @return true if t1 <= t2, false otherwise
bool operator<=(const struct timespec &t1, const struct timespec &t2);

/// @brief Greater-than-or-equal comparison for timespec structures
/// @param t1 First time value
/// @param t2 Second time value
/// @return true if t1 >= t2, false otherwise
bool operator>=(const struct timespec &t1, const struct timespec &t2);

/// @typedef ticks_t
/// @brief Time measurement in CPU ticks (uint64_t)
/// Represents a duration or timestamp in CPU ticks for high-resolution timing
typedef uint64_t ticks_t;

/// @brief Converts seconds to CPU ticks
/// Multiplies seconds by the system's ticks-per-second frequency
/// @param secs The number of seconds
/// @return The equivalent number of ticks
/// @example
/// @code
/// ticks_t one_second = secs_to_ticks(1);
/// ticks_t five_seconds = secs_to_ticks(5);
/// @endcode
ticks_t secs_to_ticks(time_t secs);

/// @brief Gets the current CPU tick count
/// Returns the elapsed number of ticks since an arbitrary epoch
/// @return The current tick count
/// @example
/// @code
/// ticks_t start = get_ticks();
/// // ... critical operation ...
/// ticks_t end = get_ticks();
/// ticks_t duration = end - start;  // Duration in ticks
/// @endcode
ticks_t get_ticks();

/// @brief Gets the current time in seconds
/// Returns the current system time as seconds since epoch
/// @return The current time in seconds
/// @example
/// @code
/// time_t now = get_secs();
/// @endcode
time_t get_secs();

/// @brief Converts CPU ticks to seconds
/// Divides ticks by the system's ticks-per-second frequency
/// @param ticks The number of ticks to convert
/// @return The equivalent time in seconds (as double for fractional seconds)
/// @example
/// @code
/// ticks_t duration = get_ticks() - start;
/// double seconds = ticks_to_secs(duration);  // Convert to seconds
/// @endcode
double ticks_to_secs(ticks_t ticks);

/// @}

#endif  // TIME_HPP_
