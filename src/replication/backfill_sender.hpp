#ifndef __REPLICATION_BACKFILL_SENDER_HPP__
#define __REPLICATION_BACKFILL_SENDER_HPP__

#include "replication/backfill.hpp"
#include "replication/protocol.hpp"

namespace replication {

/* backfill_sender_t sends backfill/realtime-streaming operations over a repli_stream_t
object. */

class backfill_sender_t :
    public backfill_and_realtime_streaming_callback_t,
    public home_thread_mixin_t
{
public:
    /* We take a pointer to a pointer to a stream; if *stream is set to NULL, then we
    stop sending things. */
    backfill_sender_t(repli_stream_t **stream);

    /* backfill_and_realtime_streaming_callback_t interface */

    void backfill_delete_range(int hash_value, int hashmod,
                               bool left_key_supplied, const store_key_t& left_key_exclusive,
                               bool right_key_supplied, const store_key_t& right_key_inclusive,
                               order_token_t token);
    void backfill_deletion(store_key_t key, order_token_t token);
    void backfill_set(backfill_atom_t atom, order_token_t token);
    void backfill_done(repli_timestamp_t timestamp_when_backfill_began, order_token_t token);

    void realtime_get_cas(const store_key_t& key, castime_t castime, order_token_t token);
    void realtime_sarc(sarc_mutation_t& m, castime_t castime, order_token_t token);
    void realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount,
        castime_t castime, order_token_t token);
    void realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key,
        const boost::shared_ptr<data_provider_t>& data, castime_t castime, order_token_t token);
    void realtime_delete_key(const store_key_t &key, repli_timestamp_t timestamp, order_token_t token);
    void realtime_time_barrier(repli_timestamp_t timestamp, order_token_t token);

private:
    repli_stream_t **const stream_;

    void warn_about_expiration();
    bool have_warned_about_expiration;

    order_sink_t order_sink_before_send;
    order_sink_t order_sink_after_send;

    template <class net_struct_type>
    void incr_decr_like(const store_key_t& key, uint64_t amount, castime_t castime);
};

}

#endif /* __REPLICATION_BACKFILL_SENDER_HPP__ */
