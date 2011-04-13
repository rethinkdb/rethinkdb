#ifndef __REPLICATION_BACKFILL_IN_HPP__
#define __REPLICATION_BACKFILL_IN_HPP__

#include "server/key_value_store.hpp"
#include "replication/backfill.hpp"
#include "replication/queueing_store.hpp"

namespace replication {

/* backfill_storer_t is the dual of the backfill_and_realtime_stream() function. It takes
the updates that backfill_and_realtime_stream() extracts from a key-value store, and it puts
them into another key-value store.

Usually, they are transmitted over the network between when they are extracted by
backfill_and_realtime_stream() and when they are stored by backfill_storer_t. */

struct backfill_storer_t : public backfill_and_realtime_streaming_callback_t {

    backfill_storer_t(btree_key_value_store_t *underlying);
    ~backfill_storer_t();

    void backfill_deletion(store_key_t key);
    void backfill_set(backfill_atom_t atom);
    void backfill_done(repli_timestamp_t timestamp);

    void realtime_get_cas(const store_key_t& key, castime_t castime);
    void realtime_sarc(const store_key_t& key, unique_ptr_t<data_provider_t> data,
        mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy,
        replace_policy_t replace_policy, cas_t old_cas);
    void realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount,
        castime_t castime);
    void realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key,
        unique_ptr_t<data_provider_t> data, castime_t castime);
    void realtime_delete_key(const store_key_t &key, repli_timestamp timestamp);
    void realtime_time_barrier(repli_timestamp_t timestamp);

private:
    bool backfilling_;
    queueing_store_t internal_store_;
};

}

#endif /* __REPLICATION_BACKFILL_IN_HPP__ */
