// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_CONSTANTS_HPP_
#define RDB_PROTOCOL_CONSTANTS_HPP_

namespace ql {

#ifndef NDEBUG
// In debug mode, always send batches of size 5.  This is essential for testing.
static inline bool should_send_batch(
    size_t batch_size_count, size_t, microtime_t) {
    return batch_size_count >= 5;
}
#else
// In release mode, send batches of 250kb, or send back however many results we
// have after 250ms, as long as we have at least one result.
static inline bool should_send_batch(
    size_t batch_size_count, size_t batch_size_bytes, microtime_t query_duration) {
    return batch_size_bytes >= MEGABYTE/4
        || (batch_size_count > 0 && query_duration > 250*1000);
}
#endif // NDEBUG

}

#endif  // RDB_PROTOCOL_CONSTANTS_HPP_
