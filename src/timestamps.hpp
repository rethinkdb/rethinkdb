// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file timestamps.hpp
/// @brief Timestamp types for versioning database state
///
/// Defines timestamp types used by the clustering code to uniquely identify
/// points in time and establish ordering of events/writes.
/// These are distinct from repli_timestamp_t which is used internally in the btree code.
///
/// @defgroup Timestamps Timestamp Management
/// Version identifiers and state change tracking
/// @{

#ifndef TIMESTAMPS_HPP_
#define TIMESTAMPS_HPP_

#include <inttypes.h>

#include <limits>

#include "containers/archive/archive.hpp"
#include "repli_timestamp.hpp"
#include "rpc/serialize_macros.hpp"

class printf_buffer_t;
class state_timestamp_t;

namespace unittest {
    /// @brief Test helper to create state timestamps
    /// Used in unit tests to construct timestamps with specific values
    /// @param n The numeric value for the timestamp
    /// @return A state_timestamp_t with value n
    state_timestamp_t make_state_timestamp(int n);
}

/// @brief Unique identifier for a point in time in the write history
///
/// Each write operation produces a new unique timestamp representing the
/// moment in logical time when that write is applied. Timestamps establish
/// a total ordering of all writes in the system.
///
/// @example
/// @code
/// state_timestamp_t before = state_timestamp_t::zero();
/// state_timestamp_t after = before.next();  // Next timestamp after write
///
/// // Establish ordering of events
/// if (write1_timestamp < write2_timestamp) {
///     // write1 happened before write2
/// }
/// @endcode
class state_timestamp_t {
public:
    /// @brief Equality comparison for timestamps
    /// @param t The timestamp to compare with
    /// @return true if both timestamps represent the same point in time
    bool operator==(state_timestamp_t t) const { return num == t.num; }

    /// @brief Inequality comparison for timestamps
    /// @param t The timestamp to compare with
    /// @return true if timestamps are different
    bool operator!=(state_timestamp_t t) const { return num != t.num; }

    /// @brief Less-than comparison for timestamps
    /// @param t The timestamp to compare with
    /// @return true if this timestamp is earlier than t
    bool operator<(state_timestamp_t t) const { return num < t.num; }

    /// @brief Greater-than comparison for timestamps
    /// @param t The timestamp to compare with
    /// @return true if this timestamp is later than t
    bool operator>(state_timestamp_t t) const { return num > t.num; }

    /// @brief Less-than-or-equal comparison for timestamps
    /// @param t The timestamp to compare with
    /// @return true if this timestamp is not later than t
    bool operator<=(state_timestamp_t t) const { return num <= t.num; }

    /// @brief Greater-than-or-equal comparison for timestamps
    /// @param t The timestamp to compare with
    /// @return true if this timestamp is not earlier than t
    bool operator>=(state_timestamp_t t) const { return num >= t.num; }

    /// @brief Creates the initial zero timestamp
    /// Represents the beginning of time before any writes
    /// @return A timestamp with value 0
    static state_timestamp_t zero() {
        state_timestamp_t t;
        t.num = 0;
        return t;
    }

    /// @brief Creates the maximum possible timestamp
    /// Represents the latest possible point in logical time
    /// @return A timestamp with maximum uint64_t value
    static state_timestamp_t max() {
        state_timestamp_t t;
        t.num = std::numeric_limits<uint64_t>::max();
        return t;
    }

    /// @brief Returns the next timestamp after this one
    /// Advances to the next point in logical time (num + 1)
    /// @return The next timestamp
    /// @example
    /// @code
    /// state_timestamp_t t1 = state_timestamp_t::zero();  // 0
    /// state_timestamp_t t2 = t1.next();  // 1
    /// @endcode
    state_timestamp_t next() const {
        state_timestamp_t t;
        t.num = num + 1;
        return t;
    }

    /// @brief Returns the previous timestamp before this one
    /// Moves back one point in logical time (num - 1)
    /// @return The previous timestamp
    /// @warning Calling on zero() will underflow
    state_timestamp_t pred() const {
        state_timestamp_t t;
        t.num = num - 1;
        return t;
    }

    // Converts a "state_timestamp_t" to a repli_timestamp_t.  Really the only
    // difference is that repli_timestamp_t::invalid exists (you shouldn't use it).
    // Also, repli_timestamp_t's are generally used in the cache and serializer,
    // where they don't necessarily come in a linear sequence -- state timestamps
    // sort of live in their shards.
    repli_timestamp_t to_repli_timestamp() const {
        repli_timestamp_t ts;
        ts.longtime = num;
        return ts;
    }

    /* Used for estimating the number of changes that have happened between two
    timestamps. */
    uint64_t count_changes(state_timestamp_t since) const {
        guarantee(*this >= since);
        return num - since.num;
    }

private:
    friend void debug_print(printf_buffer_t *buf, state_timestamp_t ts);
    friend state_timestamp_t unittest::make_state_timestamp(int);
    uint64_t num;

    RDB_MAKE_ME_SERIALIZABLE_1(state_timestamp_t, num);
};

void debug_print(printf_buffer_t *buf, state_timestamp_t ts);

#endif /* TIMESTAMPS_HPP_ */
