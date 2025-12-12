// Copyright 2010-2014 RethinkDB, all rights reserved.

/**
 * @file logger.hpp
 * @brief Logging macros and infrastructure for RethinkDB.
 *
 * Provides a set of logging macros for different severity levels: debug, info,
 * notice, warning, and error. Both printf-style and va_list variants are available.
 * Debug logging is disabled in release builds (when NDEBUG is defined).
 *
 * The logging functions are implemented in clustering/administration/logs/log_writer.cc
 * and this header exists to allow other modules to use logging without including
 * the full log writer header.
 */

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <stdarg.h>
#include <stdio.h>

#include <string>

#include "arch/compiler.hpp"

/**
 * @defgroup LoggingLevels Logging Severity Levels
 * @brief Enumeration of logging severity levels.
 */

/**
 * @ingroup LoggingLevels
 * @brief Enumeration of logging severity levels.
 *
 * Defines the standard severity levels for log messages, ordered from
 * lowest to highest severity.
 */
enum log_level_t {
    log_level_debug = 0,    ///< Debug-level messages (disabled in release builds)
    log_level_info = 1,     ///< Informational messages
    log_level_notice,       ///< Notice-level messages (normal but important)
    log_level_warn,         ///< Warning messages
    log_level_error         ///< Error messages
};

/**
 * @defgroup LoggingMacros Logging Macros
 * @brief Printf-style logging macros for different severity levels.
 *
 * These macros provide convenient logging at different severity levels.
 * The macros automatically include the source file name and line number.
 * Debug logging (logDBG/vlogDBG) is completely disabled in release builds
 * (when NDEBUG is defined) with zero runtime overhead.
 */

/**
 * @ingroup LoggingMacros
 * @brief Internal logging function for printf-style formatted messages.
 *
 * This function is called by the logging macros. Most code should use the
 * macros (logDBG, logINF, etc.) instead of calling this directly.
 *
 * @param src_file The source file name (automatically provided by macros).
 * @param src_line The source line number (automatically provided by macros).
 * @param level The logging severity level.
 * @param format Printf-style format string.
 * @param ... Variable arguments matching the format string.
 *
 * @see logDBG, logINF, logNTC, logWRN, logERR
 */
void log_internal(const char *src_file, int src_line, log_level_t level, const char *format, ...)
    ATTR_FORMAT(printf, 4, 5);

/**
 * @ingroup LoggingMacros
 * @brief Internal logging function for va_list formatted messages.
 *
 * This function is called by the va_list logging macros. Most code should use
 * the macros (vlogDBG, vlogINF, etc.) instead of calling this directly.
 *
 * @param src_file The source file name (automatically provided by macros).
 * @param src_line The source line number (automatically provided by macros).
 * @param level The logging severity level.
 * @param format Printf-style format string.
 * @param args Variable argument list matching the format string.
 *
 * @see vlogDBG, vlogINF, vlogNTC, vlogWRN, vlogERR
 */
void vlog_internal(const char *src_file, int src_line, log_level_t level, const char *format, va_list args);

/**
 * @ingroup LoggingMacros
 * @brief Logs a debug-level message with printf-style formatting.
 *
 * Logs a message at debug severity level. Debug logging is completely
 * disabled in release builds (when NDEBUG is defined) with zero overhead.
 *
 * @param fmt Printf-style format string.
 * @param ... Variable arguments matching the format string.
 *
 * @code
 * logDBG("Connection established: fd=%d from %s:%d", fd, addr, port);
 * @endcode
 *
 * @see logINF, logNTC, logWRN, logERR for other severity levels.
 */
#ifndef NDEBUG
#define logDBG(fmt, ...) log_internal(__FILE__, __LINE__, log_level_debug, (fmt), ##__VA_ARGS__)
#define vlogDBG(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_debug, (fmt), (args))
#else
#define logDBG(fmt, ...) ((void)0)
#define vlogDBG(fmt, args) ((void)0)
#endif

/**
 * @ingroup LoggingMacros
 * @brief Logs an info-level message with printf-style formatting.
 *
 * Logs a message at informational severity level.
 *
 * @param fmt Printf-style format string.
 * @param ... Variable arguments matching the format string.
 *
 * @code
 * logINF("Server started on port %d with %d worker threads", port, num_threads);
 * @endcode
 */
#define logINF(fmt, ...) log_internal(__FILE__, __LINE__, log_level_info, (fmt), ##__VA_ARGS__)

/**
 * @ingroup LoggingMacros
 * @brief Logs a notice-level message with printf-style formatting.
 *
 * Logs a message at notice severity level (normal but important events).
 *
 * @param fmt Printf-style format string.
 * @param ... Variable arguments matching the format string.
 *
 * @code
 * logNTC("Database backup completed successfully");
 * @endcode
 */
#define logNTC(fmt, ...) log_internal(__FILE__, __LINE__, log_level_notice, (fmt), ##__VA_ARGS__)

/**
 * @ingroup LoggingMacros
 * @brief Logs a warning-level message with printf-style formatting.
 *
 * Logs a message at warning severity level for potentially problematic conditions.
 *
 * @param fmt Printf-style format string.
 * @param ... Variable arguments matching the format string.
 *
 * @code
 * logWRN("Replication lag exceeds threshold: %dms (expected < %dms)",
 *        current_lag, threshold);
 * @endcode
 */
#define logWRN(fmt, ...) log_internal(__FILE__, __LINE__, log_level_warn, (fmt), ##__VA_ARGS__)

/**
 * @ingroup LoggingMacros
 * @brief Logs an error-level message with printf-style formatting.
 *
 * Logs a message at error severity level for serious problems.
 *
 * @param fmt Printf-style format string.
 * @param ... Variable arguments matching the format string.
 *
 * @code
 * logERR("Failed to write to disk: %s (errno=%d)", strerror(err), err);
 * @endcode
 */
#define logERR(fmt, ...) log_internal(__FILE__, __LINE__, log_level_error, (fmt), ##__VA_ARGS__)

/**
 * @ingroup LoggingMacros
 * @brief Logs an info-level message with va_list formatting.
 *
 * Va_list variant of logINF for use in wrapper functions that accept
 * variable arguments.
 *
 * @param fmt Printf-style format string.
 * @param args Variable argument list matching the format string.
 *
 * @see logINF for the printf-style variant.
 */
#define vlogINF(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_info, (fmt), (args))

/**
 * @ingroup LoggingMacros
 * @brief Logs a notice-level message with va_list formatting.
 *
 * @param fmt Printf-style format string.
 * @param args Variable argument list matching the format string.
 *
 * @see logNTC for the printf-style variant.
 */
#define vlogNTC(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_notice, (fmt), (args))

/**
 * @ingroup LoggingMacros
 * @brief Logs a warning-level message with va_list formatting.
 *
 * @param fmt Printf-style format string.
 * @param args Variable argument list matching the format string.
 *
 * @see logWRN for the printf-style variant.
 */
#define vlogWRN(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_warn, (fmt), (args))

/**
 * @ingroup LoggingMacros
 * @brief Logs an error-level message with va_list formatting.
 *
 * @param fmt Printf-style format string.
 * @param args Variable argument list matching the format string.
 *
 * @see logERR for the printf-style variant.
 */
#define vlogERR(fmt, args) vlog_internal(__FILE__, __LINE__, log_level_error, (fmt), (args))

#endif // LOGGER_HPP_
