// Copyright 2010-2013 RethinkDB, all rights reserved.

/// @file activity_logger.hpp
/// @brief Activity logging with timestamped events and optional stack traces
///
/// Provides classes for logging timestamped events with optional backtrace information.
/// Useful for debugging and monitoring the sequence of operations in a system.
///
/// @defgroup ActivityLogging Activity Logging
/// Event logging with timestamps and backtraces
/// @{

#ifndef ACTIVITY_LOGGER_HPP_
#define ACTIVITY_LOGGER_HPP_

#include <time.h>

#include <string>
#include <vector>

#include "backtrace.hpp"
#include "containers/scoped.hpp"
#include "threading.hpp"
#include "time.hpp"

/// @brief A single logged event with timestamp and optional stack trace
///
/// Represents a point-in-time event that can be logged with a message,
/// timestamp, and optional backtrace information.
///
/// @example
/// @code
/// log_event_t event("Processing started", true);
/// std::cout << event.print(true);  // Print with backtrace
/// @endcode
class log_event_t {
public:
    /// @brief Constructs a log event with message and optional backtrace
    /// @param _msg The event message to log
    /// @param log_bt Whether to capture a backtrace (default: true)
    explicit log_event_t(const std::string &_msg, bool log_bt = true);

    /// @brief Formats and returns the event as a printable string
    /// @param print_bt Whether to include the backtrace in output
    /// @return A formatted string representation of the event
    std::string print(bool print_bt);

private:
    /// @internal The timestamp when the event was created
    microtime_t timestamp;
    /// @internal The event message
    std::string msg;
    /// @internal The captured backtrace (if enabled)
    scoped_ptr_t<lazy_backtrace_formatter_t> bt;

    DISABLE_COPYING(log_event_t);
};

/// @brief Thread-safe activity logger for event tracking
///
/// Collects timestamped events with optional backtraces for debugging.
/// Thread-affinity enforced - must be used only from the thread that creates it.
///
/// @example
/// @code
/// activity_logger_t logger(true);  // Enable backtrace capturing
/// logger.add("Event 1");
/// logger.add("Event 2");
/// logger.add("Event 3 - without backtrace\", false);
///
/// size_t total_events = logger.size();
/// std::string log_output = logger.print(true);
/// std::cout << log_output;
/// @endcode
class activity_logger_t : private home_thread_mixin_t {
public:
    /// @brief Constructs an activity logger with optional backtrace capturing
    /// @param _log_bt Whether to capture backtraces for logged events (default: true)
    explicit activity_logger_t(bool _log_bt = true);

    /// @brief Adds an event using the backtrace setting from constructor
    /// @param msg The event message to log
    void add(const std::string &msg);

    /// @brief Adds an event with explicit backtrace control
    /// @param msg The event message to log
    /// @param log_bt Whether to capture a backtrace for this specific event
    void add(const std::string &msg, bool log_bt);

    /// @brief Returns the number of logged events
    /// @return The total count of logged events
    size_t size();

    /// @brief Prints all logged events
    /// @param print_bt Whether to include backtraces in output (default: true)
    /// @return A formatted string containing all logged events
    std::string print(bool print_bt = true);

    /// @brief Prints a range of logged events
    /// @param start The index of the first event to print (inclusive)
    /// @param end The index of the last event to print (exclusive)
    /// @param print_bt Whether to include backtraces in output (default: true)
    /// @return A formatted string containing the specified event range
    /// @example
    /// @code
    /// std::string recent = logger.print_range(5, 10, true);
    /// @endcode
    std::string print_range(size_t start, size_t end, bool print_bt = true);

private:
    /// @internal Whether to capture backtraces by default
    bool log_bt_;
    /// @internal The vector of logged events
    std::vector<scoped_ptr_t<log_event_t> > events;

    DISABLE_COPYING(activity_logger_t);
};

/// @brief No-op activity logger for performance-critical paths
///
/// A dummy activity logger that discards all events. Useful as a drop-in
/// replacement when activity logging overhead is not acceptable.
/// Still enforces thread affinity like the real logger.
///
/// @example
/// @code
/// #ifdef DEBUG_ACTIVITY
///     activity_logger_t logger;
/// #else
///     fake_activity_logger_t logger;  // No overhead in release builds
/// #endif
/// @endcode
struct fake_activity_logger_t : private home_thread_mixin_t {
    /// @brief Constructs a no-op logger
    /// @param b Unused parameter (for API compatibility)
    explicit fake_activity_logger_t(UNUSED bool b = true) { }

    /// @brief Discards a message (no-op implementation)
    /// @param msg The message that will be ignored
    void add(const std::string &msg) {
        assert_thread();
        events.push_back(msg);
    }

private:
    /// @internal Stores messages but only for thread verification
    std::vector<std::string> events;

    DISABLE_COPYING(fake_activity_logger_t);
};

/// @}

#endif /* ACTIVITY_LOGGER_HPP_ */
