// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef REPLI_TIMESTAMP_HPP_
#define REPLI_TIMESTAMP_HPP_

#include <stdint.h>

#include "containers/archive/archive.hpp"

class printf_buffer_t;


/* Note that repli_timestamp_t does NOT represent an actual timestamp; instead
it's an arbitrary counter. */

class repli_timestamp_t {
public:
    uint64_t longtime;

    bool operator==(repli_timestamp_t t) const { return longtime == t.longtime; }
    bool operator!=(repli_timestamp_t t) const { return longtime != t.longtime; }
    bool operator<(repli_timestamp_t t) const { return longtime < t.longtime; }
    bool operator>(repli_timestamp_t t) const { return longtime > t.longtime; }
    bool operator<=(repli_timestamp_t t) const { return longtime <= t.longtime; }
    bool operator>=(repli_timestamp_t t) const { return longtime >= t.longtime; }

    repli_timestamp_t next() const {
        repli_timestamp_t t;
        t.longtime = longtime + 1;
        return t;
    }

    static const repli_timestamp_t distant_past;
    static const repli_timestamp_t invalid;
};

// Returns the max of x and y, treating invalid as a negative infinity value.
repli_timestamp_t superceding_recency(repli_timestamp_t x, repli_timestamp_t y);

void serialize(write_message_t *wm, repli_timestamp_t tstamp);
archive_result_t deserialize(read_stream_t *s, repli_timestamp_t *tstamp);

void debug_print(printf_buffer_t *buf, repli_timestamp_t tstamp);

#endif  // REPLI_TIMESTAMP_HPP_
