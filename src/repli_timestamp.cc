// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <inttypes.h>

#include <algorithm>

#include "containers/archive/archive.hpp"
#include "containers/archive/versioned.hpp"
#include "containers/printf_buffer.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

template <cluster_version_t W>
void serialize(write_message_t *wm, const repli_timestamp_t &tstamp) {
    serialize<W>(wm, tstamp.longtime);
}

INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK(repli_timestamp_t);

template <cluster_version_t W>
MUST_USE archive_result_t deserialize(read_stream_t *s, repli_timestamp_t *tstamp) {
    return deserialize<W>(s, &tstamp->longtime);
}

INSTANTIATE_DESERIALIZE_SINCE_v1_13(repli_timestamp_t);

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
