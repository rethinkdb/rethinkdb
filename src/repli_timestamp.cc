#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "containers/printf_buffer.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

write_message_t &operator<<(write_message_t &msg, repli_timestamp_t tstamp) {
    return msg << tstamp.time;
}

MUST_USE archive_result_t deserialize(read_stream_t *s, repli_timestamp_t *tstamp) {
    return deserialize(s, &tstamp->time);
}

const repli_timestamp_t repli_timestamp_t::invalid = { static_cast<uint32_t>(-1) };
const repli_timestamp_t repli_timestamp_t::distant_past = { 0 };

void debug_print(append_only_printf_buffer_t *buf, repli_timestamp_t tstamp) {
    buf->appendf("%" PRIu32, tstamp.time);
}
