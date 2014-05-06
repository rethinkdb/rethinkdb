// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <inttypes.h>

#include "containers/archive/archive.hpp"
#include "containers/printf_buffer.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

void serialize(write_message_t *wm, repli_timestamp_t tstamp) {
    serialize(wm, tstamp.longtime);
}

MUST_USE archive_result_t deserialize(read_stream_t *s, repli_timestamp_t *tstamp) {
    return deserialize(s, &tstamp->longtime);
}

const repli_timestamp_t repli_timestamp_t::invalid = { UINT64_MAX };
const repli_timestamp_t repli_timestamp_t::distant_past = { 0 };

repli_timestamp_t superceding_recency(repli_timestamp_t x, repli_timestamp_t y) {
    repli_timestamp_t ret;
    // This uses the fact that invalid is UINT64_MAX.
    ret.longtime = std::max<uint64_t>(x.longtime + 1, y.longtime + 1) - 1;
    return ret;
}

void debug_print(printf_buffer_t *buf, repli_timestamp_t tstamp) {
    buf->appendf("%" PRIu64, tstamp.longtime);
}
