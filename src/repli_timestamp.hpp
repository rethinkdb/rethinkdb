// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef REPLI_TIMESTAMP_HPP_
#define REPLI_TIMESTAMP_HPP_

#include <stdint.h>

#include "containers/archive/archive.hpp"

class printf_buffer_t;


// Note that repli_timestamp_t does NOT represent an actual wall-clock time; it's an
// arbitrary counter.  This type is used in the cache and for backfilling timestamps.
// Avoid the use of repli_timestamp_t::invalid outside the cache and serializer
// (where it's mostly used for places where a timestamp doesn't exist, like if a
// block is deleted, or if a transaction is read-only).

// The values in a repli_timestamp_t correspond directly to those in a
// state_timestamp_t.  The two types are inter-convertible, _EXCEPT_ that a
// state_timestamp_t has no "invalid" value.  There is currently no function that
// converts a repli_timestamp_t to a state_timestamp_t -- the conversions only happen
// in one direction.

class repli_timestamp_t {
public:
    uint64_t longtime;

    repli_timestamp_t next() const {
        repli_timestamp_t t;
        t.longtime = longtime + 1;
        return t;
    }

    static const repli_timestamp_t distant_past;
    static const repli_timestamp_t invalid;
};

// These functions specifically make a point of passing by value, because
// repli_timestamp_t is often used in packed or misaligned structures.
inline bool operator==(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime == t.longtime; }
inline bool operator!=(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime != t.longtime; }
inline bool operator<(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime < t.longtime; }
inline bool operator>(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime > t.longtime; }
inline bool operator<=(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime <= t.longtime; }
inline bool operator>=(repli_timestamp_t s, repli_timestamp_t t) { return s.longtime >= t.longtime; }

// Returns the max of x and y, treating invalid as a negative infinity value.
repli_timestamp_t superceding_recency(repli_timestamp_t x, repli_timestamp_t y);

template <cluster_version_t W>
void serialize(write_message_t *wm, const repli_timestamp_t &tstamp);
template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s, repli_timestamp_t *tstamp);

void debug_print(printf_buffer_t *buf, repli_timestamp_t tstamp);

#endif  // REPLI_TIMESTAMP_HPP_
