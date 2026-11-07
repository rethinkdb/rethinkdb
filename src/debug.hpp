// Copyright 2010-2015 RethinkDB, all rights reserved.

/**
 * @file debug.hpp
 * @brief Debug printing and tracing utilities.
 *
 * Provides macros and functions for debug output with automatic thread information,
 * tracing function calls, and checkpoints in debug builds.
 *
 * All debug output includes the calling thread ID automatically.
 */

#ifndef DEBUG_HPP_
#define DEBUG_HPP_

#include <string>

#include "containers/printf_buffer.hpp"
#include "time.hpp"

/**
 * @defgroup DebugUtilities Debug and Tracing Utilities
 * @brief Functions and macros for debug output
 */

/**
 * @ingroup DebugUtilities
 * @def trace_call(fn, ...)
 * @brief Trace the entry and exit of a function (debug builds only).
 *
 * In debug builds, prints when the function is entered and exited.
 * In release builds, this macro expands to just calling the function.
 *
 * @param fn The function name.
 * @param ... Arguments to pass to the function.
 *
 * Example:
 * @code
 * trace_call(process_data, arg1, arg2);  // Logs entry/exit
 * @endcode
 */
#ifndef NDEBUG
#define trace_call(fn, ...) do {                                            \
        debugf("%s:%u: %s: entered\n", __FILE__, __LINE__, stringify(fn));  \
        fn(__VA_ARGS__);                                                    \
        debugf("%s:%u: %s: returned\n", __FILE__, __LINE__, stringify(fn)); \
    } while (0)

/**
 * @ingroup DebugUtilities
 * @def TRACEPOINT
 * @brief Mark a checkpoint that was reached during execution.
 *
 * In debug builds, logs the file, line number, and that this point was reached.
 * In release builds, this macro is removed entirely.
 *
 * Example:
 * @code
 * TRACEPOINT;  // Logs: filename:123 reached
 * @endcode
 */
#define TRACEPOINT debugf("%s:%u reached\n", __FILE__, __LINE__)
#else
#define trace_call(fn, ...) fn(__VA_ARGS__)
// TRACEPOINT is not defined in release, so that TRACEPOINTS do not linger in the code unnecessarily
#endif

/**
 * @ingroup DebugUtilities
 * @brief Print a quoted string for debug output.
 *
 * Handles proper escaping and quoting of binary string data.
 *
 * @param buf The printf buffer to write to.
 * @param s Pointer to the string bytes.
 * @param n Length of the string.
 */
void debug_print_quoted_string(printf_buffer_t *buf, const uint8_t *s, size_t n);

/**
 * @ingroup DebugUtilities
 * @brief Add prefix information (thread ID, timestamp) to debug output.
 *
 * Automatically called before most debug messages.
 *
 * @param buf The printf buffer to write prefix information to.
 */
void debugf_prefix_buf(printf_buffer_t *buf);

/**
 * @ingroup DebugUtilities
 * @brief Flush accumulated debug output buffer.
 *
 * @param buf The printf buffer to dump/flush.
 */
void debugf_dump_buf(printf_buffer_t *buf);

/**
 * @ingroup DebugUtilities
 * @brief Debug print for arithmetic types.
 *
 * Template specialization for built-in numeric types.
 *
 * @tparam T An arithmetic type (int, double, etc.).
 * @param buf The printf buffer to write to.
 * @param x The value to print.
 */
template <class T>
typename std::enable_if<std::is_arithmetic<T>::value>::type
debug_print(printf_buffer_t *buf, T x) {
    debug_print(buf, std::to_string(x));
}

/**
 * @ingroup DebugUtilities
 * @brief Debug print for strings.
 *
 * @param buf The printf buffer to write to.
 * @param s The string to print.
 */
void debug_print(printf_buffer_t *buf, const std::string& s);

/**
 * @ingroup DebugUtilities
 * @brief Debug print for pointers.
 *
 * @tparam T The pointed-to type.
 * @param buf The printf buffer to write to.
 * @param ptr The pointer value.
 */
template <class T>
void debug_print(printf_buffer_t *buf, T *ptr) {
    buf->appendf("%p", ptr);
}

/**
 * @ingroup DebugUtilities
 * @brief Convert an object to its debug string representation.
 *
 * Calls debug_print() and returns the result as a string.
 *
 * @tparam T The object type (must have debug_print defined).
 * @param t The object to convert.
 * @return A string representation of the object.
 *
 * Example:
 * @code
 * auto str = debug_str(some_object);
 * @endcode
 */
template<class T>
std::string debug_str(const T &t) {
    printf_buffer_t buf;
    debug_print(&buf, t);
    return buf.c_str();
}

/**
 * @ingroup DebugUtilities
 * @brief Printf-style debug output (debug builds only).
 *
 * In debug builds, prints to stderr with automatic thread/timestamp prefix.
 * In release builds, this is a no-op.
 *
 * @param msg Printf-style format string.
 * @param ... Format arguments.
 *
 * Example:
 * @code
 * debugf("Processing item %d\n", item_id);
 * @endcode
 */
#ifndef NDEBUG
void debugf(const char *msg, ...) ATTR_FORMAT(printf, 1, 2);

/**
 * @ingroup DebugUtilities
 * @brief Debug print an object with a label.
 *
 * Prints a label followed by the debug representation of an object.
 *
 * @tparam T The object type.
 * @param msg A descriptive label.
 * @param obj The object to print.
 *
 * Example:
 * @code
 * debugf_print("current_state", my_object);
 * @endcode
 */
template <class T>
void debugf_print(const char *msg, const T &obj) {
    printf_buffer_t buf;
    debugf_prefix_buf(&buf);
    buf.appendf("%s: ", msg);
    debug_print(&buf, obj);
    buf.appendf("\n");
    debugf_dump_buf(&buf);
}
#else
#define debugf(...) ((void)0)
#define debugf_print(...) ((void)0)
#endif  // NDEBUG

template <class T>
std::string debug_strprint(const T &obj) {
    printf_buffer_t buf;
    debug_print(&buf, obj);
    return std::string(buf.data(), buf.size());
}

class debugf_in_dtor_t {
public:
    explicit debugf_in_dtor_t(const char *msg, ...) ATTR_FORMAT(printf, 2, 3);
    ~debugf_in_dtor_t();
private:
    std::string message;
};

// TODO: make this more efficient (use `clock_monotonic` and use a vector of
// integers rather than accumulating a string).
class debug_timer_t {
public:
    explicit debug_timer_t(std::string _name = "");
    ~debug_timer_t();
    microtime_t tick(const std::string &tag);
private:
    microtime_t start, last;
    std::string name, out;
    DISABLE_COPYING(debug_timer_t);
};

#endif  // DEBUG_HPP_

