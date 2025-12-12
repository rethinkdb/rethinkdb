// Copyright 2010-2013 RethinkDB, all rights reserved.

/**
 * @file activity_logger.hpp
 * @brief Activity event logging with optional backtrace capture.
 *
 * Provides classes for recording timestamped events with optional stack traces,
 * useful for debugging and auditing application behavior.
 */

#ifndef ACTIVITY_LOGGER_HPP_
#define ACTIVITY_LOGGER_HPP_

#include <time.h>

#include <string>
#include <vector>

#include "backtrace.hpp"
#include "containers/scoped.hpp"
#include "threading.hpp"
#include "time.hpp"

/**
 * @defgroup ActivityLogging Activity Event Logging
 * @brief Event logging with timestamps and optional backtraces
 */

/**
 * @ingroup ActivityLogging
 * @brief A single timestamped log event.
 *
 * Captures a message and optionally a backtrace at the time of creation.
 * Can be formatted for printing with or without the backtrace.
 *
 * Example:
 * @code
 * log_event_t event("Processing started", true);  // Include backtrace
 * std::cout << event.print(true);  // Print with backtrace
 * @endcode
 */
class log_event_t {
public:
    /**
     * @brief Construct a log event with a message.
     *
     * @param _msg The event message.
     * @param log_bt Whether to capture a backtrace (default: true).
     */
    explicit log_event_t(const std::string &_msg, bool log_bt = true);

    /**
     * @brief Format the event for display.
     *
     * @param print_bt Whether to include the backtrace in output.
     * @return A formatted string representation of the event.
     */
    std::string print(bool print_bt);

private:
    microtime_t timestamp;
    std::string msg;
    scoped_ptr_t<lazy_backtrace_formatter_t> bt;

    DISABLE_COPYING(log_event_t);
};

/**
 * @ingroup ActivityLogging
 * @brief Thread-safe activity event logger.
 *
 * Maintains a circular buffer of timestamped events, each optionally including
 * a backtrace. Useful for recording a sequence of significant events for
 * debugging purposes.
 *
 * Example:
 * @code
 * activity_logger_t logger(true);  // With backtraces
 * logger.add("Event 1");
 * logger.add("Event 2");
 * logger.add("Event 3 - no backtrace", false);
 * std::cout << logger.print();  // Print all events with backtraces
 * @endcode
 */
class activity_logger_t : private home_thread_mixin_t {
public:
    /**
     * @brief Construct an activity logger.
     *
     * @param _log_bt Whether to capture backtraces by default (default: true).
     */
    explicit activity_logger_t(bool _log_bt = true);

    /**
     * @brief Add an event using the logger's default backtrace setting.
     *
     * @param msg The event message.
     */
    void add(const std::string &msg);

    /**
     * @brief Add an event with explicit backtrace setting.
     *
     * @param msg The event message.
     * @param log_bt Whether to capture a backtrace for this event.
     */
    void add(const std::string &msg, bool log_bt);

    /**
     * @brief Get the number of events logged.
     *
     * @return The count of events.
     */
    size_t size();

    /**
     * @brief Get a formatted string of all logged events.
     *
     * @param print_bt Whether to include backtraces (default: true).
     * @return A formatted string containing all events.
     */
    std::string print(bool print_bt = true);

    /**
     * @brief Get a formatted string of events in a range.
     *
     * @param start The starting index (inclusive).
     * @param end The ending index (exclusive).
     * @param print_bt Whether to include backtraces (default: true).
     * @return A formatted string containing events in the range.
     */
    std::string print_range(size_t start, size_t end, bool print_bt = true);

private:
    bool log_bt_;
    std::vector<scoped_ptr_t<log_event_t> > events;

    DISABLE_COPYING(activity_logger_t);
};

/**
 * @ingroup ActivityLogging
 * @brief Stub logger for when activity logging is disabled.
 *
 * Has the same interface as activity_logger_t but does no actual logging.
 * Useful for compile-time disabling of logging overhead.
 */
struct fake_activity_logger_t : private home_thread_mixin_t {
    /**
     * @brief Construct a fake logger (no-op).
     *
     * @param b Ignored, present for API compatibility.
     */
    explicit fake_activity_logger_t(UNUSED bool b = true) { }

    /**
     * @brief Add an event (no-op, but validates thread).
     *
     * @param msg The event message (ignored).
     */
    void add(const std::string &msg) {
        assert_thread();
        events.push_back(msg);
    }

private:
    std::vector<std::string> events;

    DISABLE_COPYING(fake_activity_logger_t);
};

#endif /* ACTIVITY_LOGGER_HPP_ */
