/**
 * @file rethinkdb_backtrace.hpp
 * @brief Low-level backtrace capture for RethinkDB.
 *
 * Provides a minimal interface to capture raw stack traces. This is used internally
 * by the higher-level backtrace.hpp utilities.
 */

#ifndef RETHINKDB_BACKTRACE_HPP_
#define RETHINKDB_BACKTRACE_HPP_

/**
 * @defgroup LowLevelBacktrace Low-Level Backtrace Capture
 * @brief Raw stack trace capture utilities.
 */

/**
 * @ingroup LowLevelBacktrace
 * @brief Number of internal stack frames to skip in rethinkdb_backtrace().
 *
 * This constant specifies how many frames at the top of the stack should be
 * removed from the backtrace to hide the call to rethinkdb_backtrace() itself.
 * Adjust this if the implementation of rethinkdb_backtrace() changes.
 *
 * @note This value is specific to the current implementation of rethinkdb_backtrace().
 *       If the implementation is modified, please verify and update this constant.
 */
#define NUM_FRAMES_INSIDE_RETHINKDB_BACKTRACE   1

/**
 * @ingroup LowLevelBacktrace
 * @brief Captures the current call stack.
 *
 * Raw backtrace capture function that stores instruction pointers from the
 * current call stack into a buffer. This is a low-level function; most code
 * should use the higher-level backtrace_t and format_backtrace() functions
 * from backtrace.hpp instead.
 *
 * @param buffer Array to store the captured instruction pointers.
 * @param size Maximum number of frames to capture (size of buffer / sizeof(void*)).
 * @return The actual number of frames captured, which may be less than @p size
 *         if the stack is shorter. Returns 0 if backtraces are disabled
 *         (RDB_NO_BACKTRACE) or if capture fails.
 *
 * @code
 * void* frames[64];
 * int num_frames = rethinkdb_backtrace(frames, 64);
 * for (int i = 0; i < num_frames; ++i) {
 *     // Process frames[i]
 * }
 * @endcode
 *
 * @note This function captures raw instruction pointers. Use backtrace_t or
 *       format_backtrace() for human-readable output.
 * @note The first NUM_FRAMES_INSIDE_RETHINKDB_BACKTRACE frames should be
 *       skipped to remove the internal call to rethinkdb_backtrace() itself.
 * @note This function is a wrapper around the platform-specific backtrace
 *       mechanisms (e.g., backtrace() on Linux, StackWalk64 on Windows).
 */

#endif  // RETHINKDB_BACKTRACE_HPP_
