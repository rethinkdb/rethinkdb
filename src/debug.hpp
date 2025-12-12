// Copyright 2010-2015 RethinkDB, all rights reserved.

/// @file debug.hpp
/// @brief Debugging utilities and tracing functions for RethinkDB
///
/// This module provides compile-time controlled debug printing and execution
/// tracing. All debug functions are conditionally compiled and can be completely
/// eliminated in release builds (when NDEBUG is defined).
///
/// @defgroup DebugUtilities Debug and Trace Utilities
/// Runtime debugging output and execution tracing
/// @{

#ifndef DEBUG_HPP_
#define DEBUG_HPP_

#include <string>

#include "containers/printf_buffer.hpp"
#include "time.hpp"

/// @brief Traces function entry and exit with optional parameters
/// Outputs debug messages on function entry and exit
/// @note This macro is disabled in release builds (NDEBUG defined)
/// @param fn The function to trace
/// @param ... Variable arguments passed to the function
/// @example
/// @code
/// void process_data(int value) {
///     // ... function implementation
/// }
/// trace_call(process_data, 42);  // Logs entry/exit messages
/// @endcode
#ifndef NDEBUG
#define trace_call(fn, ...) do {                                            \
        debugf("%s:%u: %s: entered\n", __FILE__, __LINE__, stringify(fn));  \
        fn(__VA_ARGS__);                                                    \
        debugf("%s:%u: %s: returned\n", __FILE__, __LINE__, stringify(fn)); \
    } while (0)

/// @brief Marks a point in code execution for tracing
/// Outputs the current file and line number to debug output
/// @note This macro is disabled in release builds (NDEBUG defined)
/// @example
/// @code
/// int main() {
///     TRACEPOINT;  // Outputs: "/path/to/file.cc:42 reached\n"
///     // ... rest of code
/// }
/// @endcode
#define TRACEPOINT debugf("%s:%u reached\n", __FILE__, __LINE__)
#else
#define trace_call(fn, ...) fn(__VA_ARGS__)
// TRACEPOINT is not defined in release, so that TRACEPOINTS do not linger in the code unnecessarily
#endif

/// @brief Outputs a debug string with escaped quotes and special characters
/// @param buf The output buffer to write to
/// @param s Pointer to the string data to print
/// @param n The length of the string in bytes
void debug_print_quoted_string(printf_buffer_t *buf, const uint8_t *s, size_t n);

/// @brief Outputs the standard debug prefix (thread ID, timestamp, etc.)
/// @param buf The output buffer to write to
void debugf_prefix_buf(printf_buffer_t *buf);

/// @brief Flushes and outputs the contents of the debug buffer
/// @param buf The output buffer to dump
void debugf_dump_buf(printf_buffer_t *buf);

/// @brief Debug prints an arithmetic type (int, float, double, etc.)
/// Specialization for arithmetic types that converts to string
/// @tparam T An arithmetic type (integral or floating point)
/// @param buf The output buffer to write to
/// @param x The value to print
/// @example
/// @code
/// int value = 42;
/// debug_print(&buffer, value);  // Prints "42"
/// double pi = 3.14159;
/// debug_print(&buffer, pi);  // Prints "3.14159"
/// @endcode
template <class T>
typename std::enable_if<std::is_arithmetic<T>::value>::type
debug_print(printf_buffer_t *buf, T x) {
    debug_print(buf, std::to_string(x));
}

/// @brief Debug prints a std::string to the output buffer
/// @param buf The output buffer to write to
/// @param s The string to print
void debug_print(printf_buffer_t *buf, const std::string& s);

/// @brief Debug prints a pointer as a hexadecimal address
/// @tparam T The type of pointer
/// @param buf The output buffer to write to
/// @param ptr The pointer value to print
/// @example
/// @code
/// int* ptr = new int(5);
/// debug_print(&buffer, ptr);  // Prints "0x7fff5fbff7c0" or similar
/// @endcode
template <class T>
void debug_print(printf_buffer_t *buf, T *ptr) {
    buf->appendf("%p", ptr);
}

/// @brief Converts a debuggable object to a string representation
/// Helper template that creates a string representation of any object
/// that has a debug_print specialization
/// @tparam T The type of object to convert
/// @param t The object to convert to string
/// @return A string representation of the object
/// @example
/// @code
/// std::string result = debug_str(42);  // Returns "42"
/// std::string str_result = debug_str(std::string("hello"));  // Returns "hello"
/// @endcode
template<class T>
std::string debug_str(const T &t) {
    printf_buffer_t buf;
    debug_print(&buf, t);
    return buf.c_str();
}

/// @}

#ifndef NDEBUG
void debugf(const char *msg, ...) ATTR_FORMAT(printf, 1, 2);
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

