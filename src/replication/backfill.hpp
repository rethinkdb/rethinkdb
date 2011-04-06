#ifndef __REPLICATION_BACKFILL_HPP__
#define __REPLICATION_BACKFILL_HPP__

#include "btree/backfill.hpp"
#include "server/key_value_store.hpp"

namespace replication {

struct backfill_and_realtime_streaming_callback_t {

    virtual void backfill_deletion(store_key_t key) = 0;
    virtual void backfill_set(backfill_atom_t atom) = 0;
    virtual void backfill_done(repli_timestamp_t timestamp_when_backfill_began) = 0;

    virtual void realtime_get_cas(const store_key_t& key, castime_t castime) = 0;
    virtual void realtime_sarc(const store_key_t& key, unique_ptr_t<data_provider_t> data,
        mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy,
        replace_policy_t replace_policy, cas_t old_cas) = 0;
    virtual void realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount,
        castime_t castime) = 0;
    virtual void realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key,
        unique_ptr_t<data_provider_t> data, castime_t castime) = 0;
    virtual void realtime_delete_key(const store_key_t &key, repli_timestamp timestamp) = 0;
    virtual void realtime_time_barrier(repli_timestamp_t timestamp) = 0;
};

/* Call backfill_and_realtime_stream() to stream data from the given key-value store to the
given backfill_and_realtime_streaming_callback_t. It will begin by concurrently streaming backfill
and realtime updates, then call backfill_done() and just stream realtime updates.

When you pulse the multicond_t that you give it (from another coroutine) it will stop streaming
and return. */

void backfill_and_realtime_stream(

    /* The key-value store to get the data out of. */
    btree_key_value_store_t *kvs,

    /* The time from which to start backfilling. */
    repli_timestamp_t start_time,

    /* The place to send the backfill information and real-time streaming operations. */
    backfill_and_realtime_streaming_callback_t *cb,

    /* If you pulse this multicond, then backfill_and_realtime_stream() will (eventually) return.
    Because it's impossible to interrupt a backfill, it won't return until the backfill is done. */
    multicond_t *pulse_to_stop
);

}  // namespace replication



#endif  // __REPLICATION_BACKFILL_HPP__
