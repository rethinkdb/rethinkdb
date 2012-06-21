#ifndef REPLI_TIMESTAMP_HPP_
#define REPLI_TIMESTAMP_HPP_

#include <stdint.h>

#include "containers/archive/archive.hpp"

class append_only_printf_buffer_t;

/* Note that repli_timestamp_t does NOT represent an actual timestamp; instead it's an arbitrary
counter. */

class repli_timestamp_t {
public:
    uint32_t time;

    bool operator==(repli_timestamp_t t) const { return time == t.time; }
    bool operator!=(repli_timestamp_t t) const { return time != t.time; }
    bool operator<(repli_timestamp_t t) const { return time < t.time; }
    bool operator>(repli_timestamp_t t) const { return time > t.time; }
    bool operator<=(repli_timestamp_t t) const { return time <= t.time; }
    bool operator>=(repli_timestamp_t t) const { return time >= t.time; }

    repli_timestamp_t next() const {
        repli_timestamp_t t;
        t.time = time + 1;
        return t;
    }

    static const repli_timestamp_t distant_past;
    static const repli_timestamp_t invalid;
};

write_message_t &operator<<(write_message_t &msg, repli_timestamp_t tstamp);
archive_result_t deserialize(read_stream_t *s, repli_timestamp_t *tstamp);

// LIKE std::max, except it's technically not associative.
// TODO: repli_max is silly and obsolete.
repli_timestamp_t repli_max(repli_timestamp_t x, repli_timestamp_t y);

void debug_print(append_only_printf_buffer_t *buf, repli_timestamp_t tstamp);

#endif  // REPLI_TIMESTAMP_HPP_
