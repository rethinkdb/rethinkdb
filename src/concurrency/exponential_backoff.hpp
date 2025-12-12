// Copyright 2010-2015 RethinkDB, all rights reserved.

/// @file exponential_backoff.hpp
/// @brief Exponential backoff implementation for retry scenarios
///
/// Provides an exponential backoff strategy for automatically managing delays
/// between retry attempts. Increases delay on failures and decreases on successes
/// within configured bounds.
///
/// @defgroup ConcurrencyControl
/// @{

#ifndef CONCURRENCY_EXPONENTIAL_BACKOFF_HPP_
#define CONCURRENCY_EXPONENTIAL_BACKOFF_HPP_

#include "arch/timing.hpp"

/// @brief Implements exponential backoff with configurable bounds
///
/// `exponential_backoff_t` manages retry delays that grow exponentially
/// with consecutive failures and shrink with successes. This helps handle
/// transient failures (network timeouts, temporary unavailability) by
/// gradually increasing wait times while avoiding resource exhaustion.
///
/// Useful for:
/// - Retrying network operations
/// - Database connection re-establishment
/// - Rate limiting after failures
///
/// @example
/// @code
/// // Create backoff with 100ms min, 30s max, 1.5x failure growth
/// exponential_backoff_t backoff(100, 30000, 1.5, 0.5);
///
/// while (!connected) {
///     try {
///         connect_to_server();
///         backoff.success();  // Reset delay on success
///     } catch (const connection_error &e) {
///         backoff.failure(interruptor);  // Wait with exponential backoff
///     }
/// }
/// @endcode
///
/// @note The fail_factor and success_factor should satisfy:
///       - fail_factor > 1.0 (to increase delay on failures)
///       - 0.0 <= success_factor < 1.0 (to decrease delay on successes)
class exponential_backoff_t {
public:
    /// @brief Constructs an exponential backoff controller
    /// @param _mnbm Minimum backoff time in milliseconds
    /// @param _mxbm Maximum backoff time in milliseconds
    /// @param _ff Failure factor: multiply delay by this on each failure (default 1.5)
    /// @param _sf Success factor: multiply delay by this on success (default 0.0 = reset)
    exponential_backoff_t(
            uint64_t _mnbm, uint64_t _mxbm, double _ff = 1.5, double _sf = 0.0) :
        min_backoff_ms(_mnbm), max_backoff_ms(_mxbm), fail_factor(_ff),
        success_factor(_sf), backoff_ms(0)
        { }

    /// @brief Records a failure and applies exponential backoff
    /// Increases the backoff delay, then yields/sleeps for that duration.
    /// On first failure, yields briefly. On subsequent failures, sleeps with exponential increase.
    /// @param interruptor Signal to interrupt the backoff sleep (e.g., shutdown signal)
    /// @post backoff_ms will be increased by fail_factor, capped at max_backoff_ms
    void failure(signal_t *interruptor) {
        if (backoff_ms == 0) {
            coro_t::yield();
            backoff_ms = min_backoff_ms;
        } else {
            nap(backoff_ms, interruptor);
            guarantee(static_cast<uint64_t>(backoff_ms * fail_factor) > backoff_ms,
                "rounding screwed it up");
            backoff_ms *= fail_factor;
            if (backoff_ms > max_backoff_ms) {
                backoff_ms = max_backoff_ms;
            }
        }
    }

    /// @brief Records a success and reduces backoff delay
    /// Decreases the backoff delay, resetting to 0 if success_factor is 0.0.
    /// @post backoff_ms will be decreased by success_factor, reset if < min_backoff_ms
    void success() {
        guarantee(static_cast<uint64_t>(backoff_ms * success_factor) < backoff_ms
            || backoff_ms == 0, "rounding screwed it up");
        backoff_ms *= success_factor;
        if (backoff_ms < min_backoff_ms) {
            backoff_ms = 0;
        }
    }

private:
    const uint64_t min_backoff_ms;      ///< Minimum delay in milliseconds
    const uint64_t max_backoff_ms;      ///< Maximum delay in milliseconds
    const double fail_factor;           ///< Multiplier for delay on failure
    const double success_factor;        ///< Multiplier for delay on success
    uint64_t backoff_ms;                ///< Current backoff delay
};

/// @}

#endif /* CONCURRENCY_EXPONENTIAL_BACKOFF_HPP_ */

