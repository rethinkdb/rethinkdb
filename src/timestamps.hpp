// Copyright 2010-2014 RethinkDB, all rights reserved.

/**
 * @file timestamps.hpp
 * @brief State timestamp management for clustering and replication.
 *
 * Defines state_timestamp_t, a unique identifier for points in the write
 * sequence. Each write carries a timestamp identifying the new time point
 * that exists when the write is applied.
 */

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
    state_timestamp_t make_state_timestamp(int n);
}

/**
 * @defgroup Timestamps Timestamp Management
 * @brief State and replication timestamp utilities
 */

/**
 * @ingroup Timestamps
 * @brief A unique identifier for a point in a series of writes.
 *
 * state_timestamp_t represents a logical point in time. Each write operation
 * carries the state_timestamp_t that will exist when that write is applied,
 * making timestamps unique identifiers in the write history.
 *
 * Timestamps support:
 * - Comparison operations (==, !=, <, >, <=, >=)
 * - Zero and maximum sentinel values
 * - Successor (next()) and predecessor (pred()) operations
 * - Conversion to/from repli_timestamp_t
 * - Serialization
 *
 * Example:
 * @code
 * state_timestamp_t t1 = state_timestamp_t::zero();
 * state_timestamp_t t2 = t1.next();
 * state_timestamp_t t3 = t2.next();
 *
 * if (t2 > t1) {
 *     uint64_t changes = t3.count_changes(t1);  // Returns 2
 * }
 * @endcode
 */
class state_timestamp_t {
public:
    /**
     * @brief Equality comparison.
     *
     * @param t The timestamp to compare with.
     * @return true if both timestamps are equal.
     */
    bool operator==(state_timestamp_t t) const { return num == t.num; }

    /**
     * @brief Inequality comparison.
     *
     * @param t The timestamp to compare with.
     * @return true if timestamps are not equal.
     */
    bool operator!=(state_timestamp_t t) const { return num != t.num; }

    /**
     * @brief Less-than comparison.
     *
     * @param t The timestamp to compare with.
     * @return true if this timestamp is less than t.
     */
    bool operator<(state_timestamp_t t) const { return num < t.num; }

    /**
     * @brief Greater-than comparison.
     *
     * @param t The timestamp to compare with.
     * @return true if this timestamp is greater than t.
     */
    bool operator>(state_timestamp_t t) const { return num > t.num; }

    /**
     * @brief Less-than-or-equal comparison.
     *
     * @param t The timestamp to compare with.
     * @return true if this timestamp is <= t.
     */
    bool operator<=(state_timestamp_t t) const { return num <= t.num; }

    /**
     * @brief Greater-than-or-equal comparison.
     *
     * @param t The timestamp to compare with.
     * @return true if this timestamp is >= t.
     */
    bool operator>=(state_timestamp_t t) const { return num >= t.num; }

    /**
     * @brief The earliest/zero timestamp.
     *
     * @return A timestamp with value 0.
     */
    static state_timestamp_t zero() {
        state_timestamp_t t;
        t.num = 0;
        return t;
    }

    /**
     * @brief The maximum possible timestamp.
     *
     * @return A timestamp with the maximum uint64_t value.
     */
    static state_timestamp_t max() {
        state_timestamp_t t;
        t.num = std::numeric_limits<uint64_t>::max();
        return t;
    }

    /**
     * @brief Get the successor timestamp.
     *
     * @return A timestamp one greater than this.
     *
     * Example:
     * @code
     * state_timestamp_t t0 = state_timestamp_t::zero();
     * state_timestamp_t t1 = t0.next();  // t1 > t0
     * @endcode
     */
    state_timestamp_t next() const {
        state_timestamp_t t;
        t.num = num + 1;
        return t;
    }

    /**
     * @brief Get the predecessor timestamp.
     *
     * @return A timestamp one less than this.
     *
     * Example:
     * @code
     * state_timestamp_t t1 = state_timestamp_t::zero().next();
     * state_timestamp_t t0 = t1.pred();  // Back to zero
     * @endcode
     */
    state_timestamp_t pred() const {
        state_timestamp_t t;
        t.num = num - 1;
        return t;
    }

    /**
     * @brief Convert to repli_timestamp_t for internal btree/cache operations.
     *
     * repli_timestamp_t is used internally within the btree code and cache/serializer,
     * while state_timestamp_t is used by the clustering layer. The primary difference
     * is that repli_timestamp_t::invalid exists as a sentinel value.
     *
     * @return The equivalent repli_timestamp_t.
     *
     * Example:
     * @code
     * state_timestamp_t st = state_timestamp_t::zero();
     * repli_timestamp_t rt = st.to_repli_timestamp();
     * @endcode
     */
    repli_timestamp_t to_repli_timestamp() const {
        repli_timestamp_t ts;
        ts.longtime = num;
        return ts;
    }

    /**
     * @brief Estimate the number of changes between two timestamps.
     *
     * Returns the difference in timestamp values, which estimates how many
     * write operations occurred between the two points.
     *
     * @param since The older timestamp to measure from.
     * @return The estimated number of changes: this->num - since->num.
     *
     * @pre This timestamp must be >= since.
     *
     * Example:
     * @code
     * state_timestamp_t old = state_timestamp_t::zero();
     * state_timestamp_t new_ts = old.next().next().next();  // 3 changes later
     * assert(new_ts.count_changes(old) == 3);
     * @endcode
     */
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

/**
 * @ingroup Timestamps
 * @brief Debug print a state_timestamp_t to a printf buffer.
 *
 * @param buf The printf buffer to write to.
 * @param ts The timestamp to print.
 */
void debug_print(printf_buffer_t *buf, state_timestamp_t ts);

#endif /* TIMESTAMPS_HPP_ */
