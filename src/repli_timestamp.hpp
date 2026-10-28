// Copyright 2010-2012 RethinkDB, all rights reserved.

/**
 * @file repli_timestamp.hpp
 * @brief Replication timestamps for cache and serializer operations.
 *
 * Defines repli_timestamp_t, a logical timestamp counter used internally
 * in the cache and serializer. Unlike state_timestamp_t, repli_timestamp_t
 * includes an "invalid" sentinel value for representing missing/deleted blocks.
 *
 * @note repli_timestamp_t is NOT a wall-clock time; it's an arbitrary counter.
 */

#ifndef REPLI_TIMESTAMP_HPP_
#define REPLI_TIMESTAMP_HPP_

#include <stdint.h>

#include "containers/archive/archive.hpp"

class printf_buffer_t;

/**
 * @defgroup ReplicationTimestamps Replication Timestamps
 * @brief Internal timestamp management for cache and serializer
 */

/**
 * @ingroup ReplicationTimestamps
 * @brief A logical timestamp for cache and serializer operations.
 *
 * repli_timestamp_t is an arbitrary counter used internally within the cache
 * and serializer layers. Unlike state_timestamp_t (used by clustering), this
 * type includes an "invalid" sentinel value.
 *
 * Key differences from state_timestamp_t:
 * - Includes repli_timestamp_t::invalid for missing/deleted blocks
 * - Includes repli_timestamp_t::distant_past sentinel
 * - Used in packed/misaligned structures (passed by value)
 * - Values correspond directly to state_timestamp_t values (1:1 mapping)
 * - Can convert state_timestamp_t -> repli_timestamp_t (but not vice versa)
 *
 * Uses:
 * - Marking timestamp of cache blocks
 * - Tracking read-only transaction timestamps
 * - Serialization and backfilling operations
 *
 * Example:
 * @code
 * repli_timestamp_t ts = repli_timestamp_t::distant_past;
 * repli_timestamp_t next_ts = ts.next();
 *
 * if (ts == repli_timestamp_t::invalid) {
 *     // Block was deleted or transaction is read-only
 * }
 * @endcode
 */
class repli_timestamp_t {
public:
    /**
     * @brief The internal timestamp value.
     *
     * A 64-bit unsigned integer representing the logical time.
     */
    uint64_t longtime;

    /**
     * @brief Get the next/successor timestamp.
     *
     * @return A timestamp one greater than this.
     *
     * Example:
     * @code
     * repli_timestamp_t ts1 = ...;
     * repli_timestamp_t ts2 = ts1.next();
     * @endcode
     */
    repli_timestamp_t next() const {
        repli_timestamp_t t;
        t.longtime = longtime + 1;
        return t;
    }

    /**
     * @brief Sentinel for the distant past / earliest timestamp.
     *
     * Represents the beginning of logical time in a replication context.
     */
    static const repli_timestamp_t distant_past;

    /**
     * @brief Sentinel for invalid/missing timestamps.
     *
     * Used to represent:
     * - Deleted blocks (no valid timestamp)
     * - Read-only transactions (no write timestamp)
     * - Missing or uninitialized timestamps
     */
    static const repli_timestamp_t invalid;
};

/**
 * @ingroup ReplicationTimestamps
 * @brief Equality comparison.
 *
 * @param s First timestamp.
 * @param t Second timestamp.
 * @return true if both timestamps are equal.
 *
 * @note Passed by value for safe use in packed/misaligned structures.
 */
inline bool operator==(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime == t.longtime; }

/**
 * @ingroup ReplicationTimestamps
 * @brief Inequality comparison.
 *
 * @param s First timestamp.
 * @param t Second timestamp.
 * @return true if timestamps differ.
 *
 * @note Passed by value for safe use in packed/misaligned structures.
 */
inline bool operator!=(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime != t.longtime; }

/**
 * @ingroup ReplicationTimestamps
 * @brief Less-than comparison.
 *
 * @param s First timestamp.
 * @param t Second timestamp.
 * @return true if s < t.
 *
 * @note Passed by value for safe use in packed/misaligned structures.
 */
inline bool operator<(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime < t.longtime; }

/**
 * @ingroup ReplicationTimestamps
 * @brief Greater-than comparison.
 *
 * @param s First timestamp.
 * @param t Second timestamp.
 * @return true if s > t.
 *
 * @note Passed by value for safe use in packed/misaligned structures.
 */
inline bool operator>(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime > t.longtime; }

/**
 * @ingroup ReplicationTimestamps
 * @brief Less-than-or-equal comparison.
 *
 * @param s First timestamp.
 * @param t Second timestamp.
 * @return true if s <= t.
 *
 * @note Passed by value for safe use in packed/misaligned structures.
 */
inline bool operator<=(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime <= t.longtime; }

/**
 * @ingroup ReplicationTimestamps
 * @brief Greater-than-or-equal comparison.
 *
 * @param s First timestamp.
 * @param t Second timestamp.
 * @return true if s >= t.
 *
 * @note Passed by value for safe use in packed/misaligned structures.
 */
inline bool operator>=(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime >= t.longtime; }

/**
 * @ingroup ReplicationTimestamps
 * @brief Find the more recent of two timestamps.
 *
 * Returns the maximum of x and y, treating repli_timestamp_t::invalid as
 * negative infinity. Useful for determining which timestamp supersedes another
 * in replication scenarios.
 *
 * @param x First timestamp candidate.
 * @param y Second timestamp candidate.
 * @return The timestamp that is more recent (or more valid).
 *
 * Example:
 * @code
 * repli_timestamp_t ts1 = get_timestamp();
 * repli_timestamp_t ts2 = other_timestamp();
 * repli_timestamp_t merged = superceding_recency(ts1, ts2);
 * @endcode
 */
repli_timestamp_t superceding_recency(repli_timestamp_t x, repli_timestamp_t y);

/**
 * @ingroup ReplicationTimestamps
 * @brief Serialize a repli_timestamp_t to a write message.
 *
 * @tparam W The cluster version to serialize for.
 * @param wm The write message to serialize to.
 * @param tstamp The timestamp to serialize.
 */
template <cluster_version_t W>
void serialize(write_message_t *wm, const repli_timestamp_t &tstamp);

/**
 * @ingroup ReplicationTimestamps
 * @brief Deserialize a repli_timestamp_t from a read stream.
 *
 * @tparam W The cluster version to deserialize from.
 * @param s The read stream to deserialize from.
 * @param tstamp Pointer to receive the deserialized timestamp.
 * @return Archive result indicating success/failure.
 */
template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s, repli_timestamp_t *tstamp);

/**
 * @ingroup ReplicationTimestamps
 * @brief Debug print a repli_timestamp_t to a printf buffer.
 *
 * @param buf The printf buffer to write to.
 * @param tstamp The timestamp to print.
 */
void debug_print(printf_buffer_t *buf, repli_timestamp_t tstamp);

#endif  // REPLI_TIMESTAMP_HPP_
