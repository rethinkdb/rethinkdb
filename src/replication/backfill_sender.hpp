#ifndef __REPLICATION_BACKFILL_SENDER_HPP__
#define __REPLICATION_BACKFILL_SENDER_HPP__

#include "replication/backfill.hpp"
#include "replication/protocol.hpp"

namespace replication {

/* backfill_sender_t sends backfill/realtime-streaming operations over a repli_stream_t
object. */

struct backfill_sender_t :
    public backfill_and_realtime_streaming_callback_t,
    public home_thread_mixin_t
{
    /* We take a pointer to a pointer to a stream; if *stream is set to NULL, then we
    stop sending things. */
    backfill_sender_t(repli_stream_t **stream);

    /* backfill_and_realtime_streaming_callback_t interface */

    void backfill_delete_everything();
    void backfill_deletion(store_key_t key);
    void backfill_set(backfill_atom_t atom);
    void backfill_done(repli_timestamp_t timestamp_when_backfill_began);

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
    repli_stream_t **const stream_;

    void warn_about_expiration();
    bool have_warned_about_expiration;

    template <class net_struct_type>
    void incr_decr_like(const store_key_t& key, uint64_t amount, castime_t castime);
};

}

#endif /* __REPLICATION_BACKFILL_SENDER_HPP__ */
