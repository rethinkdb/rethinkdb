#include "replication/backfill_sender.hpp"

#include "logger.hpp"
#include "perfmon.hpp"

namespace replication {
perfmon_duration_sampler_t
    master_bf_del("master_bf_del", secs_to_ticks(1.0)),
    master_bf_set("master_bf_set", secs_to_ticks(1.0)),
    master_rt_op("master_rt_op", secs_to_ticks(1.0)),
    master_rt_timebarrier("master_rt_timebarrier", secs_to_ticks(5.0));

backfill_sender_t::backfill_sender_t(repli_stream_t **stream) :
    stream_(stream), have_warned_about_expiration(false) { }

void backfill_sender_t::warn_about_expiration() {
    if (!have_warned_about_expiration) {
        logWRN("RethinkDB does not support the combination of expiration times and replication. "
            "The master and the slave may report different values for keys that have expiration "
            "times.\n");
        have_warned_about_expiration = true;
    }
}

void backfill_sender_t::backfill_delete_range(int hash_value, int hashmod,
                                              bool left_key_supplied, const store_key_t& left_key_exclusive,
                                              bool right_key_supplied, const store_key_t& right_key_inclusive,
                                              order_token_t token) {
    order_sink_before_send.check_out(token);

    if (*stream_) {
        int left_offseter = (left_key_supplied ? left_key_exclusive.size : 0);
        size_t n = sizeof(net_backfill_delete_range_t)
            + left_offseter
            + (right_key_supplied ? right_key_inclusive.size : 0);

        scoped_malloc<net_backfill_delete_range_t> msg(n);
        msg->hash_value = hash_value;
        msg->hashmod = hashmod;
        msg->low_key_size = left_key_supplied ? left_key_exclusive.size : net_backfill_delete_range_t::infinity_key_size;
        msg->high_key_size = right_key_supplied ? right_key_inclusive.size : net_backfill_delete_range_t::infinity_key_size;
        if (left_key_supplied) {
            memcpy(msg->keys, left_key_exclusive.contents, left_key_exclusive.size);
        }
        if (right_key_supplied) {
            memcpy(msg->keys + left_offseter, right_key_inclusive.contents, right_key_inclusive.size);
        }

        (*stream_)->send(msg.get());
    }

    order_sink_after_send.check_out(token);
}

void backfill_sender_t::backfill_deletion(store_key_t key, order_token_t token) {
    block_pm_duration del_timer(&master_bf_del);

    order_sink_before_send.check_out(token);

    if (*stream_) {
        size_t n = sizeof(net_backfill_delete_t) + key.size;

        scoped_malloc<net_backfill_delete_t> msg(n);
        msg->padding = 0;
        msg->key_size = key.size;
        memcpy(msg->key, key.contents, key.size);

        (*stream_)->send(msg.get());
    }

    order_sink_after_send.check_out(token);
}

void backfill_sender_t::backfill_set(backfill_atom_t atom, order_token_t token) {
    assert_thread();

    block_pm_duration set_timer(&master_bf_set);

    order_sink_before_send.check_out(token);

    if (atom.exptime != 0) {
        warn_about_expiration();
    }

    if (*stream_) {
        net_backfill_set_t msg;
        msg.timestamp = atom.recency;
        msg.flags = atom.flags;
        msg.exptime = atom.exptime;
        msg.cas_or_zero = atom.cas_or_zero;
        msg.key_size = atom.key.size;
        msg.value_size = atom.value->size();
        (*stream_)->send(&msg, atom.key.contents, atom.value);
    }

    order_sink_after_send.check_out(token);
}

void backfill_sender_t::backfill_done(repli_timestamp_t timestamp_when_backfill_began, order_token_t token) {
    order_sink_before_send.check_out(token);

    net_backfill_complete_t msg;
    msg.time_barrier_timestamp = timestamp_when_backfill_began;
    if (*stream_) (*stream_)->send(&msg);

    order_sink_after_send.check_out(token);
}

void backfill_sender_t::realtime_get_cas(const store_key_t& key, castime_t castime, order_token_t token) {
    assert_thread();

    block_pm_duration set_timer(&master_rt_op);

    order_sink_before_send.check_out(token);

    if (*stream_) {
        size_t n = sizeof(net_get_cas_t) + key.size;
        scoped_malloc<net_get_cas_t> msg(n);
        msg->proposed_cas = castime.proposed_cas;
        msg->timestamp = castime.timestamp;
        msg->key_size = key.size;

        memcpy(msg->key, key.contents, key.size);

        if (*stream_) (*stream_)->send(msg.get());
    }

    order_sink_after_send.check_out(token);
}

void backfill_sender_t::realtime_sarc(sarc_mutation_t& m, castime_t castime, order_token_t token) {
    assert_thread();

    block_pm_duration set_timer(&master_rt_op);

    order_sink_before_send.check_out(token);

    if (m.exptime != 0) {
        warn_about_expiration();
    }

    if (*stream_) {
        net_sarc_t stru;
        stru.timestamp = castime.timestamp;
        stru.proposed_cas = castime.proposed_cas;
        stru.flags = m.flags;
        stru.exptime = m.exptime;
        stru.key_size = m.key.size;
        stru.value_size = m.data->size();
        stru.add_policy = m.add_policy;
        stru.replace_policy = m.replace_policy;
        stru.old_cas = m.old_cas;
        if (*stream_) (*stream_)->send(&stru, m.key.contents, m.data);
    }

    order_sink_after_send.check_out(token);
}

void backfill_sender_t::realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, castime_t castime, order_token_t token) {
    assert_thread();

    block_pm_duration set_timer(&master_rt_op);

    order_sink_before_send.check_out(token);

    if (*stream_) {
        if (kind == incr_decr_INCR) {
            incr_decr_like<net_incr_t>(key, amount, castime);
        } else {
            rassert(kind == incr_decr_DECR);
            incr_decr_like<net_decr_t>(key, amount, castime);
        }
    }

    order_sink_after_send.check_out(token);
}

template <class net_struct_type>
void backfill_sender_t::incr_decr_like(const store_key_t &key, uint64_t amount, castime_t castime) {
    size_t n = sizeof(net_struct_type) + key.size;

    assert_thread();

    scoped_malloc<net_struct_type> msg(n);
    msg->timestamp = castime.timestamp;
    msg->proposed_cas = castime.proposed_cas;
    msg->amount = amount;
    msg->key_size = key.size;
    memcpy(msg->key, key.contents, key.size);

    if (*stream_) (*stream_)->send(msg.get());
}

void backfill_sender_t::realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key, const boost::intrusive_ptr<data_buffer_t>& data, castime_t castime, order_token_t token) {
    assert_thread();
    block_pm_duration set_timer(&master_rt_op);
    order_sink_before_send.check_out(token);

    if (*stream_) {
        if (kind == append_prepend_APPEND) {
            net_append_t appendstruct;
            appendstruct.timestamp = castime.timestamp;
            appendstruct.proposed_cas = castime.proposed_cas;
            appendstruct.key_size = key.size;
            appendstruct.value_size = data->size();

            if (*stream_) (*stream_)->send(&appendstruct, key.contents, data);
        } else {
            rassert(kind == append_prepend_PREPEND);

            net_prepend_t prependstruct;
            prependstruct.timestamp = castime.timestamp;
            prependstruct.proposed_cas = castime.proposed_cas;
            prependstruct.key_size = key.size;
            prependstruct.value_size = data->size();

            if (*stream_) (*stream_)->send(&prependstruct, key.contents, data);
        }
    }
    order_sink_after_send.check_out(token);
}

void backfill_sender_t::realtime_delete_key(const store_key_t &key, repli_timestamp_t timestamp, order_token_t token) {
    assert_thread();
    block_pm_duration set_timer(&master_rt_op);
    order_sink_before_send.check_out(token);

    size_t n = sizeof(net_delete_t) + key.size;
    if (*stream_) {
        scoped_malloc<net_delete_t> msg(n);
        msg->timestamp = timestamp;
        msg->key_size = key.size;
        memcpy(msg->key, key.contents, key.size);

        if (*stream_) (*stream_)->send(msg.get());
    }

    order_sink_after_send.check_out(token);
}

void backfill_sender_t::realtime_time_barrier(repli_timestamp_t timestamp, order_token_t token) {
    block_pm_duration timer(&master_rt_timebarrier);
    order_sink_before_send.check_out(token);
    assert_thread();
    net_timebarrier_t msg;
    msg.timestamp = timestamp;
    if (*stream_) (*stream_)->send(msg);
    order_sink_after_send.check_out(token);
}

}
