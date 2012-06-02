#include "utils.hpp"
#include "repli_timestamp.hpp"

write_message_t &operator<<(write_message_t &msg, repli_timestamp_t tstamp) {
    return msg << tstamp.time;
}

MUST_USE archive_result_t deserialize(read_stream_t *s, repli_timestamp_t *tstamp) {
    return deserialize(s, &tstamp->time);
}

const repli_timestamp_t repli_timestamp_t::invalid = { static_cast<uint32_t>(-1) };
const repli_timestamp_t repli_timestamp_t::distant_past = { 0 };

repli_timestamp_t repli_max(repli_timestamp_t x, repli_timestamp_t y) {
    return int32_t(x.time - y.time) < 0 ? y : x;
}
