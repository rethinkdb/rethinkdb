// Copyright 2010-2012 RethinkDB, all rights reserved.

/**
 * @file repli_timestamp.hpp
 * @brief Replication timestamp types for tracking version information.
 *
 * Defines the repli_timestamp_t class, which represents a logical counter rather than
 * an actual wall-clock time. Used in the cache and for backfilling timestamps to track
 * changes in replication.
 */

#ifndef REPLI_TIMESTAMP_HPP_
#define REPLI_TIMESTAMP_HPP_

#include <stdint.h>

#include "containers/archive/archive.hpp"

class printf_buffer_t;

/**
 * @defgroup ReplicationTimestamps Replication Timestamp Types
 * @brief Logical timestamp types for tracking version information in replication.
 */


/**
 * @ingroup ReplicationTimestamps
 * @brief Logical timestamp counter for replication and caching.
 *
 * Represents a monotonically increasing logical counter used to track versions
 * of data in replication and cache scenarios. This is NOT an actual wall-clock time.
 *
 * The repli_timestamp_t corresponds directly to state_timestamp_t values, but
 * includes an additional "invalid" sentinel value for use in caching contexts.
 *
 * @note Use repli_timestamp_t::invalid sparingly, mostly for cache and serializer
 *       where timestamps don't exist (e.g., deleted blocks or read-only transactions).
 *
 * @code
 * // Typical usage for tracking data versions
 * repli_timestamp_t original_version;
 * repli_timestamp_t newer_version = original_version.next();
 * assert(newer_version > original_version);
 *
 * // Check if timestamp is valid
 * if (ts != repli_timestamp_t::invalid) {
 *     // Use this timestamp
 * }
 * @endcode
 */
class repli_timestamp_t {
public:
    /**
     * @brief The underlying counter value.
     *
     * This 64-bit integer is the core of the timestamp. Values are ordered,
     * with larger values representing newer versions.
     */
    uint64_t longtime;

    /**
     * @brief Returns the next logical timestamp.
     *
     * Increments the timestamp by one to represent the next version.
     *
     * @return A new repli_timestamp_t with longtime incremented by 1.
     *
     * @code
     * repli_timestamp_t t1; t1.longtime = 100;
     * repli_timestamp_t t2 = t1.next();
     * assert(t2.longtime == 101);
     * @endcode
     */
    repli_timestamp_t next() const {
        repli_timestamp_t t;
        t.longtime = longtime + 1;
        return t;
    }

    /**
     * @brief The earliest (minimum) valid timestamp.
     *
     * Represents the beginning of time for replication purposes.
     */
    static const repli_timestamp_t distant_past;

    /**
     * @brief A sentinel value indicating no valid timestamp.
     *
     * Used to represent cases where no timestamp exists (e.g., when a block
     * is deleted or a transaction is read-only). Treats this value as
     * negative infinity in comparisons with superceding_recency().
     */
    static const repli_timestamp_t invalid;
};

/**
 * @ingroup ReplicationTimestamps
 * @brief Comparison operators for repli_timestamp_t.
 *
 * These operators allow repli_timestamp_t values to be compared.
 * Note that comparison is done by value, since repli_timestamp_t is small
 * and often used in packed or misaligned structures.
 *
 * @{
 */

/**
 * @brief Equality comparison.
 */
inline bool operator==(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime == t.longtime; }

/**
 * @brief Inequality comparison.
 */
inline bool operator!=(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime != t.longtime; }

/**
 * @brief Less-than comparison.
 */
inline bool operator<(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime < t.longtime; }

/**
 * @brief Greater-than comparison.
 */
inline bool operator>(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime > t.longtime; }

/**
 * @brief Less-than-or-equal comparison.
 */
inline bool operator<=(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime <= t.longtime; }

/**
 * @brief Greater-than-or-equal comparison.
 */
inline bool operator>=(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime >= t.longtime; }
/** @} */

/**
 * @ingroup ReplicationTimestamps
 * @brief Returns the maximum of two timestamps, treating invalid as negative infinity.
 *
 * This function is useful when determining the most recent version between two
 * timestamps, while correctly handling invalid (non-existent) timestamps.
 *
 * @param x First timestamp to compare.
 * @param y Second timestamp to compare.
 * @return The greater of the two timestamps, with the property that:
 *         - If either is invalid, returns the other
 *         - If both are invalid, returns invalid
 *         - Otherwise, returns the larger value
 *
 * @code
 * auto t1 = repli_timestamp_t::invalid;
 * auto t2; t2.longtime = 100;
 * auto result = superceding_recency(t1, t2);
 * assert(result.longtime == 100);
 *
 * auto t3; t3.longtime = 50;
 * auto t4; t4.longtime = 200;
 * assert(superceding_recency(t3, t4).longtime == 200);
 * @endcode
 */
repli_timestamp_t superceding_recency(repli_timestamp_t x, repli_timestamp_t y);

/**
 * @ingroup ReplicationTimestamps
 * @brief Serializes a repli_timestamp_t for network transmission or storage.
 *
 * @tparam W The cluster protocol version to use for serialization.
 * @param wm The write message to serialize into.
 * @param tstamp The timestamp to serialize.
 */
template <cluster_version_t W>
void serialize(write_message_t *wm, const repli_timestamp_t &tstamp);

/**
 * @ingroup ReplicationTimestamps
 * @brief Deserializes a repli_timestamp_t from network transmission or storage.
 *
 * @tparam W The cluster protocol version to use for deserialization.
 * @param s The read stream to deserialize from.
 * @param tstamp Output parameter: the deserialized timestamp.
 * @return An archive_result_t indicating success or failure.
 */
template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s, repli_timestamp_t *tstamp);

/**
 * @ingroup ReplicationTimestamps
 * @brief Pretty-prints a repli_timestamp_t for debugging.
 *
 * @param buf The printf_buffer_t to write output to.
 * @param tstamp The timestamp to print.
 */
void debug_print(printf_buffer_t *buf, repli_timestamp_t tstamp);

#endif  // REPLI_TIMESTAMP_HPP_
