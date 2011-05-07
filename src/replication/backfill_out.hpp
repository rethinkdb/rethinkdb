#ifndef __REPLICATION_BACKFILL_OUT_HPP__
#define __REPLICATION_BACKFILL_OUT_HPP__

#include "btree/backfill.hpp"
#include "server/key_value_store.hpp"
#include "replication/backfill.hpp"

namespace replication {

/* Call backfill_and_realtime_stream() to stream data from the given key-value store to the
given backfill_and_realtime_streaming_callback_t. It will begin by concurrently streaming backfill
and realtime updates, then call backfill_done() and just stream realtime updates.

When you pulse the signal_t that you give it (from another coroutine) it will stop streaming
and return. */

void backfill_and_realtime_stream(

    /* The key-value store to get the data out of. */
    btree_key_value_store_t *kvs,

    /* The time from which to start backfilling. */
    repli_timestamp_t start_time,

    /* The place to send the backfill information and real-time streaming operations. */
    backfill_and_realtime_streaming_callback_t *cb,

    /* If you pulse this signal, then backfill_and_realtime_stream() will (eventually) return.
    Because it's impossible to interrupt a backfill, it won't return until the backfill is done. */
    signal_t *pulse_to_stop
);

}  // namespace replication



#endif  // __REPLICATION_BACKFILL_OUT_HPP__
