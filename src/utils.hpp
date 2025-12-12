// Copyright 2010-2015 RethinkDB, all rights reserved.

/**
 * @file utils.hpp
 * @brief Utility functions for system operations, string manipulation, and time formatting.
 *
 * This header provides fundamental utility functions used throughout the RethinkDB
 * codebase, including cryptographic random number generation, string parsing, time
 * formatting, and logging helpers.
 */

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <functional>
#include <string>

#include "arch/compiler.hpp"
#include "config/args.hpp"
#include "errors.hpp"

#ifdef _MSC_VER
#include <BaseTsd.h>
#include <random>
typedef SSIZE_T ssize_t;
#endif

class printf_buffer_t;

namespace ph = std::placeholders;

/**
 * @defgroup UtilsInit Initialization and Shutdown
 * @brief System initialization and cleanup utilities.
 */

/**
 * @ingroup UtilsInit
 * @brief RAII wrapper for startup and shutdown operations.
 *
 * This class manages startup and shutdown sequences for the RethinkDB system.
 * Constructing this object initializes the system, while destruction cleans up
 * resources.
 */
class startup_shutdown_t {
public:
    /**
     * @brief Initializes the RethinkDB system.
     *
     * Performs necessary startup operations such as thread pool initialization,
     * signal handler setup, and other system-level initialization.
     */
    startup_shutdown_t();

    /**
     * @brief Cleans up RethinkDB system resources.
     *
     * Performs graceful shutdown of all system components and resources.
     */
    ~startup_shutdown_t();
};

/**
 * @brief A lightweight string slice type representing a character range.
 *
 * This structure provides a non-owning reference to a string buffer through
 * begin and end pointers, similar to std::string_view.
 *
 * @code
 * const char* str = "hello world";
 * const_charslice slice(str, str + 5);
 * // slice represents the string "hello"
 * @endcode
 */
struct const_charslice {
    const char *beg;  ///< Pointer to the beginning of the string range
    const char *end;  ///< Pointer to one past the last character

    /**
     * @brief Constructs a charslice with the given range.
     * @param _beg Beginning of the character range.
     * @param _end One past the end of the character range.
     */
    const_charslice(const char *_beg, const char *_end) : beg(_beg), end(_end) { }

    /**
     * @brief Constructs a null/empty charslice.
     */
    const_charslice() : beg(nullptr), end(nullptr) { }
};

enum class query_state_t { FAILED, INDETERMINATE };

/**
 * @defgroup UtilsCrypto Cryptographic Functions
 * @brief Secure random number generation.
 */

/**
 * @ingroup UtilsCrypto
 * @brief Generates cryptographically secure random bytes.
 *
 * Reads from /dev/urandom (or equivalent) to generate random bytes suitable
 * for cryptographic purposes. Use sparingly for performance reasons.
 *
 * @param out Pointer to output buffer where random bytes will be stored.
 * @param nbytes Number of random bytes to generate.
 *
 * @code
 * uint8_t buffer[32];
 * system_random_bytes(buffer, sizeof(buffer));
 * // buffer now contains 32 cryptographically secure random bytes
 * @endcode
 *
 * @warning This function performs I/O and may block. Use judiciously in
 *          performance-critical sections.
 * @warning The output buffer must be at least @p nbytes in size.
 */
void system_random_bytes(void *out, int64_t nbytes);

/**
 * @defgroup UtilsStringParsing String Parsing Utilities
 * @brief Functions for parsing strings into numeric values.
 */

/**
 * @ingroup UtilsStringParsing
 * @brief Checks if a string begins with a minus sign.
 * @param string The string to check (null-terminated).
 * @return true if the string starts with '-', false otherwise.
 */
bool begins_with_minus(const char *string);

/**
 * @ingroup UtilsStringParsing
 * @brief Strictly parses a C-string to a 64-bit signed integer.
 *
 * Unlike standard strtol/strtoll functions, this implementation correctly
 * rejects negative values when parsing unsigned integers.
 *
 * @param string The null-terminated string to parse.
 * @param end Output parameter: pointer to the first unparsed character.
 * @param base The base for number interpretation (2-36, or 0 for auto-detection).
 * @return The parsed 64-bit signed integer value.
 *
 * @code
 * const char* str = "12345";
 * const char* end;
 * int64_t value = strtoi64_strict(str, &end, 10);
 * // value = 12345, end points to the null terminator
 * @endcode
 *
 * @see strtou64_strict for unsigned integer parsing.
 */
int64_t strtoi64_strict(const char *string, const char **end, int base);

/**
 * @ingroup UtilsStringParsing
 * @brief Strictly parses a C-string to a 64-bit unsigned integer.
 *
 * Rejects strings beginning with a minus sign, addressing a deficiency in
 * the standard strtoul and strtoull functions.
 *
 * @param string The null-terminated string to parse.
 * @param end Output parameter: pointer to the first unparsed character.
 * @param base The base for number interpretation (2-36, or 0 for auto-detection).
 * @return The parsed 64-bit unsigned integer value.
 *
 * @code
 * const char* str = "9999999999";
 * const char* end;
 * uint64_t value = strtou64_strict(str, &end, 10);
 * // value = 9999999999, end points to the null terminator
 * @endcode
 *
 * @see strtoi64_strict for signed integer parsing.
 */
uint64_t strtou64_strict(const char *string, const char **end, int base);

/**
 * @ingroup UtilsStringParsing
 * @brief Parses a std::string to a 64-bit signed integer.
 *
 * This overload requires the entire string to be consumed by the conversion.
 * Returns false if conversion fails or if unparsed characters remain.
 *
 * @param str The string to parse.
 * @param base The base for number interpretation (2-36, or 0 for auto-detection).
 * @param out_result Output parameter: the parsed integer value (set to 0 on failure).
 * @return true if the entire string was successfully parsed, false otherwise.
 *
 * @code
 * int64_t result;
 * if (strtoi64_strict("42", 10, &result)) {
 *     // result contains 42
 * } else {
 *     // conversion failed
 * }
 * @endcode
 *
 * @note The @c MUST_USE macro indicates this result should not be discarded.
 */
MUST_USE bool strtoi64_strict(const std::string &str, int base, int64_t *out_result);

/**
 * @ingroup UtilsStringParsing
 * @brief Parses a std::string to a 64-bit unsigned integer.
 *
 * This overload requires the entire string to be consumed by the conversion.
 * Returns false if conversion fails, if unparsed characters remain, or if the
 * string begins with a minus sign.
 *
 * @param str The string to parse.
 * @param base The base for number interpretation (2-36, or 0 for auto-detection).
 * @param out_result Output parameter: the parsed integer value (set to 0 on failure).
 * @return true if the entire string was successfully parsed, false otherwise.
 *
 * @code
 * uint64_t result;
 * if (strtou64_strict("18446744073709551615", 10, &result)) {
 *     // result contains maximum uint64_t
 * }
 * @endcode
 *
 * @note The @c MUST_USE macro indicates this result should not be discarded.
 */
MUST_USE bool strtou64_strict(const std::string &str, int base, uint64_t *out_result);

/**
 * @defgroup UtilsFormatting String Formatting
 * @brief Printf-style string formatting utilities.
 */

/**
 * @ingroup UtilsFormatting
 * @brief Formats a string using printf-style formatting.
 *
 * Safely formats a string with variable arguments, similar to printf but
 * returning a std::string instead of writing to a file.
 *
 * @param format Printf-style format string.
 * @param ... Variable arguments to substitute in the format string.
 * @return The formatted string.
 *
 * @code
 * std::string msg = strprintf("Error: %d occurred in %s", errno, strerror(errno));
 * @endcode
 *
 * @see vstrprintf for va_list variant.
 */
std::string strprintf(const char *format, ...) ATTR_FORMAT(printf, 1, 2);

/**
 * @ingroup UtilsFormatting
 * @brief Formats a string using printf-style formatting with a va_list.
 *
 * This is the va_list variant of strprintf, useful for implementing wrapper
 * functions that accept variable arguments.
 *
 * @param format Printf-style format string.
 * @param ap Variable argument list.
 * @return The formatted string.
 *
 * @code
 * void log_message(const char* format, ...) {
 *     va_list args;
 *     va_start(args, format);
 *     std::string msg = vstrprintf(format, args);
 *     va_end(args);
 *     // process msg...
 * }
 * @endcode
 */
std::string vstrprintf(const char *format, va_list ap) ATTR_FORMAT(printf, 1, 0);


/**
 * @defgroup UtilsTime Time Formatting and Parsing
 * @brief Functions for formatting and parsing timestamps.
 */

/**
 * @ingroup UtilsTime
 * @brief Length of a formatted ISO 8601 timestamp string (not including null terminator).
 *
 * Formatted timestamps have the structure: yyyy-mm-ddThh:mm:ss.nnnnnnnnn
 * This constant specifies the exact length without the null terminator.
 */
const size_t formatted_time_length = 29;    // not including null

/**
 * @ingroup UtilsTime
 * @brief Specifies whether to use local or UTC time.
 */
enum class local_or_utc_time_t { local, utc };

/**
 * @ingroup UtilsTime
 * @brief Formats a timespec to an ISO 8601 timestamp string.
 *
 * Writes the time in ISO 8601 format (yyyy-mm-ddThh:mm:ss.nnnnnnnnn) to the
 * provided printf_buffer_t.
 *
 * @param time The timespec structure to format.
 * @param buf The printf_buffer_t to write the formatted time to.
 * @param zone Whether to use local or UTC time for the conversion.
 *
 * @code
 * struct timespec ts = {1234567890, 123456789};
 * printf_buffer_t buf;
 * format_time(ts, &buf, local_or_utc_time_t::utc);
 * // buf now contains "2009-02-13T23:31:30.123456789"
 * @endcode
 */
void format_time(struct timespec time, printf_buffer_t *buf, local_or_utc_time_t zone);

/**
 * @ingroup UtilsTime
 * @brief Formats a timespec to an ISO 8601 timestamp string (std::string variant).
 *
 * @param time The timespec structure to format.
 * @param zone Whether to use local or UTC time for the conversion.
 * @return The formatted timestamp string in ISO 8601 format.
 *
 * @code
 * auto now = std::chrono::system_clock::now();
 * auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
 * struct timespec ts = {time_t(ns / 1000000000), long(ns % 1000000000)};
 * std::string timestamp = format_time(ts, local_or_utc_time_t::utc);
 * @endcode
 */
std::string format_time(struct timespec time, local_or_utc_time_t zone);

/**
 * @ingroup UtilsTime
 * @brief Parses an ISO 8601 timestamp string.
 *
 * Parses a timestamp string in ISO 8601 format and converts it to a timespec.
 *
 * @param str The timestamp string to parse (e.g., "2009-02-13T23:31:30.123456789").
 * @param zone Whether the string represents local or UTC time.
 * @param out Output parameter: the parsed timespec structure.
 * @param errmsg_out Output parameter: error message if parsing fails (may be nullptr).
 * @return true if parsing succeeded, false otherwise.
 *
 * @code
 * struct timespec ts;
 * std::string errmsg;
 * if (parse_time("2009-02-13T23:31:30.123456789", local_or_utc_time_t::utc, &ts, &errmsg)) {
 *     // ts now contains the parsed timestamp
 * } else {
 *     // errmsg contains the error description
 * }
 * @endcode
 */
bool parse_time(
    const std::string &str, local_or_utc_time_t zone,
    struct timespec *out, std::string *errmsg_out);

/**
 * @defgroup UtilsDebug Debug Utilities
 * @brief Debugging and diagnostics helpers.
 */

/**
 * @ingroup UtilsDebug
 * @brief Prints a hex dump of binary data to stderr.
 *
 * Outputs the binary data in a human-readable hexadecimal format, suitable
 * for debugging and diagnostics. The output includes both hex representation
 * and ASCII representation where applicable.
 *
 * @param buf Pointer to the binary data to dump.
 * @param offset Starting offset to display in the output.
 * @param length Number of bytes to dump.
 *
 * @code
 * const char* data = "\x00\x01\x02\xFF\xFE";
 * print_hd(data, 0, 5);
 * // Prints something like:
 * // 0000: 00 01 02 FF FE   .....
 * @endcode
 */
void print_hd(const void *buf, size_t offset, size_t length);



/**
 * @ingroup UtilsInit
 * @brief RAII wrapper for temporarily changing coroutine priority.
 *
 * This class implements the RAII pattern to safely manage temporary changes
 * to the current coroutine's priority. When constructed, it saves the current
 * priority and sets a new one. When destructed, it restores the original priority.
 *
 * This is useful for ensuring that priority-sensitive operations are executed
 * at the appropriate priority level, with automatic restoration on scope exit.
 *
 * @code
 * {
 *     with_priority_t high_priority(10);  // Raise priority to 10
 *     // Perform critical operation...
 * }  // Priority automatically restored here
 * @endcode
 */
class with_priority_t {
public:
    /**
     * @brief Changes the current coroutine's priority.
     * @param priority The new priority level to set.
     */
    explicit with_priority_t(int priority);

    /**
     * @brief Restores the original priority.
     */
    ~with_priority_t();
private:
    int previous_priority;  ///< The saved priority value to restore
};

/**
 * @ingroup UtilsFormatting
 * @brief Converts an errno value to a human-readable error string.
 *
 * @param errsv The errno value to convert (e.g., from errno or strerror).
 * @return A string description of the error.
 *
 * @code
 * if (open(filename, O_RDONLY) == -1) {
 *     std::string msg = errno_string(errno);
 *     // msg might be "No such file or directory"
 * }
 * @endcode
 */
std::string errno_string(int errsv);

/**
 * @ingroup UtilsFormatting
 * @brief Sanitizes a string for safe logging output.
 *
 * Removes or escapes characters that might cause issues in log output,
 * such as control characters or newlines.
 *
 * @param s The string to sanitize.
 * @return A sanitized version of the string safe for logging.
 */
std::string sanitize_for_logger(const std::string &s);

/**
 * @ingroup UtilsTime
 * @brief Converts a time_t to a formatted time string.
 *
 * Wrapper around ctime_r (POSIX) or ctime_s (Windows) for thread-safe
 * time formatting. The returned string includes the date, time, and timezone.
 *
 * @param t The time_t value to format.
 * @return A formatted time string (e.g., "Mon Jan 02 15:04:05 2006").
 *
 * @code
 * time_t now = time(NULL);
 * std::string timestr = time2str(now);
 * // timestr might be "Fri Dec 13 14:30:45 2024"
 * @endcode
 */
static inline std::string time2str(const time_t &t) {
    const int TIMEBUF_SIZE = 26; // As specified in man 3 ctime and by MSDN
    char timebuf[TIMEBUF_SIZE];
#ifdef _WIN32
    errno_t ret = ctime_s(timebuf, sizeof(timebuf), &t);
    guarantee_err(ret == 0, "time2str: invalid time");
    return timebuf;
#else
    return ctime_r(&t, timebuf);
#endif
}

#define MSTR(x) stringify(x) // Stringify a macro
#if defined __clang__
#define COMPILER_STR "CLANG " __clang_version__
#elif defined __GNUC__
#define COMPILER_STR "GCC " MSTR(__GNUC__) "." MSTR(__GNUC_MINOR__) "." MSTR(__GNUC_PATCHLEVEL__)
#elif defined _MSC_VER
#define COMPILER_STR "MSC " MSTR(_MSC_FULL_VER)
#else
#define COMPILER_STR "UNKNOWN COMPILER"
#endif

#define RETHINKDB_VERSION_STR_TRAILER " (" BUILD_MACHINE ")" " (" COMPILER_STR ")"

#ifndef NDEBUG
#define RETHINKDB_VERSION_STR "rethinkdb " RETHINKDB_VERSION " (debug)" RETHINKDB_VERSION_STR_TRAILER
#else
#define RETHINKDB_VERSION_STR "rethinkdb " RETHINKDB_VERSION RETHINKDB_VERSION_STR_TRAILER
#endif

#define ANY_PORT 0

template <class T>
T clone(const T& x) {
    return x;
}

#endif // UTILS_HPP_
