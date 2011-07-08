#ifndef __REPLICATION_BACKFILL_HPP__
#define __REPLICATION_BACKFILL_HPP__

#include "btree/backfill.hpp"
#include "concurrency/fifo_checker.hpp"

namespace replication {

struct backfill_and_realtime_streaming_callback_t {

    virtual void backfill_delete_everything(order_token_t token) = 0;
    virtual void backfill_deletion(store_key_t key, order_token_t token) = 0;
    virtual void backfill_set(backfill_atom_t atom, order_token_t token) = 0;

    // `backfill_done()` is called when all the backfilled changes with timestamps less than
    // `timestamp` have been sent.
    virtual void backfill_done(repli_timestamp_t timestamp, order_token_t token) = 0;

    virtual void realtime_get_cas(const store_key_t& key, castime_t castime, order_token_t token) = 0;
    virtual void realtime_sarc(sarc_mutation_t& m, castime_t castime, order_token_t token) = 0;
    virtual void realtime_sarc(const store_key_t& key, boost::shared_ptr<data_provider_t> data,
                               mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy,
                               replace_policy_t replace_policy, cas_t old_cas, order_token_t token) {
        sarc_mutation_t m;
        m.key = key;
        m.data = data;
        m.flags = flags;
        m.exptime = exptime;
        m.add_policy = add_policy;
        m.replace_policy = replace_policy;
        m.old_cas = old_cas;
        realtime_sarc(m, castime, token);
    }
    virtual void realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount,
                                    castime_t castime, order_token_t token) = 0;
    virtual void realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key,
                                         boost::shared_ptr<data_provider_t> data, castime_t castime, order_token_t token) = 0;
    virtual void realtime_delete_key(const store_key_t &key, repli_timestamp_t timestamp, order_token_t token) = 0;

    // `realtime_time_barrier()` is called when all the realtime changes with timestamps less than
    // `timestamp` have been sent.
    virtual void realtime_time_barrier(repli_timestamp_t timestamp, order_token_t token) = 0;

protected:
    virtual ~backfill_and_realtime_streaming_callback_t() { }
};

}

#endif /* __REPLICATION_BACKFILL_HPP__ */
