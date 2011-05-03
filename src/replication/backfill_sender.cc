#include "replication/backfill_sender.hpp"

namespace replication {
perfmon_duration_sampler_t
    master_del("master_bf_del", secs_to_ticks(1.0)),
    master_set("master_bf_set", secs_to_ticks(1.0));

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

void backfill_sender_t::backfill_delete_everything() {
    debugf("send backfill_delete_everything(), %d\n", int(bool(*stream_)));

    if (*stream_) {
        net_backfill_delete_everything_t msg;
        msg.padding = 0;
        (*stream_)->send(msg);
    }
}

void backfill_sender_t::backfill_deletion(store_key_t key) {
    block_pm_duration set_timer(&master_del);

    debugf("send backfill_deletion(%.*s), %d\n", key.size, key.contents, int(bool(*stream_)));

    size_t n = sizeof(net_backfill_delete_t) + key.size;
    if (*stream_) {
        scoped_malloc<net_backfill_delete_t> msg(n);
        msg->padding = 0;
        msg->key_size = key.size;
        memcpy(msg->key, key.contents, key.size);

        (*stream_)->send(msg.get());
    }
}

void backfill_sender_t::backfill_set(backfill_atom_t atom) {
    block_pm_duration set_timer(&master_set);

    debugf("send backfill_set(%.*s, t=%u, len=%ld), %d\n", atom.key.size, atom.key.contents, atom.recency.time, atom.value->get_size(), int(bool(*stream_)));

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
        msg.value_size = atom.value->get_size();
        (*stream_)->send(&msg, atom.key.contents, atom.value);
    }

    debugf("done send backfill_set(%.*s), %d\n", atom.key.size, atom.key.contents, int(bool(*stream_)));
}

void backfill_sender_t::backfill_done(repli_timestamp_t timestamp_when_backfill_began) {

    debugf("send backfill_done(), %d\n", int(bool(*stream_)));

    net_backfill_complete_t msg;
    msg.time_barrier_timestamp = timestamp_when_backfill_began;
    if (*stream_) (*stream_)->send(&msg);
}

void backfill_sender_t::realtime_get_cas(const store_key_t& key, castime_t castime) {
    assert_thread();

    debugf("send realtime_get_cas(%.*s), %d\n", key.size, key.contents, int(bool(*stream_)));

    if (*stream_) {
        size_t n = sizeof(net_get_cas_t) + key.size;
        scoped_malloc<net_get_cas_t> msg(n);
        msg->proposed_cas = castime.proposed_cas;
        msg->timestamp = castime.timestamp;
        msg->key_size = key.size;

        memcpy(msg->key, key.contents, key.size);

        if (*stream_) (*stream_)->send(msg.get());
    }
}

void backfill_sender_t::realtime_sarc(const store_key_t& key, unique_ptr_t<data_provider_t> data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    assert_thread();

    debugf("send realtime_sarc(%.*s), %d\n", key.size, key.contents, int(bool(*stream_)));

    if (exptime != 0) {
        warn_about_expiration();
    }

    if (*stream_) {
        net_sarc_t stru;
        stru.timestamp = castime.timestamp;
        stru.proposed_cas = castime.proposed_cas;
        stru.flags = flags;
        stru.exptime = exptime;
        stru.key_size = key.size;
        stru.value_size = data->get_size();
        stru.add_policy = add_policy;
        stru.replace_policy = replace_policy;
        stru.old_cas = old_cas;
        try {
            if (*stream_) (*stream_)->send(&stru, key.contents, data);
        } catch (data_provider_failed_exc_t) {
            /* Do nothing. Because the data provider failed, the operation was never performed
            on the master, so it's good if it's also never performed on the slave either. */
        }
    }

    debugf("done send realtime_sarc(%.*s), %d\n", key.size, key.contents, int(bool(*stream_)));
}

void backfill_sender_t::realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, castime_t castime) {
    assert_thread();

    debugf("send realtime_incr_decr(%.*s), %d\n", key.size, key.contents, int(bool(*stream_)));

    if (*stream_) {
        if (kind == incr_decr_INCR) {
            incr_decr_like<net_incr_t>(key, amount, castime);
        } else {
            rassert(kind == incr_decr_DECR);
            incr_decr_like<net_decr_t>(key, amount, castime);
        }
    }
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

void backfill_sender_t::realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key, unique_ptr_t<data_provider_t> data, castime_t castime) {
    assert_thread();

    debugf("send realtime_append_prepend(%.*s), %d\n", key.size, key.contents, int(bool(*stream_)));

    if (*stream_) {
        if (kind == append_prepend_APPEND) {
            net_append_t appendstruct;
            appendstruct.timestamp = castime.timestamp;
            appendstruct.proposed_cas = castime.proposed_cas;
            appendstruct.key_size = key.size;
            appendstruct.value_size = data->get_size();

            try {
                if (*stream_) (*stream_)->send(&appendstruct, key.contents, data);
            } catch (data_provider_failed_exc_t) {
                /* See coment in realtime_sarc() */
            }
        } else {
            rassert(kind == append_prepend_PREPEND);

            net_prepend_t prependstruct;
            prependstruct.timestamp = castime.timestamp;
            prependstruct.proposed_cas = castime.proposed_cas;
            prependstruct.key_size = key.size;
            prependstruct.value_size = data->get_size();

            try {
                if (*stream_) (*stream_)->send(&prependstruct, key.contents, data);
            } catch (data_provider_failed_exc_t) {
                /* See coment in realtime_sarc() */
            }
        }
    }
}

void backfill_sender_t::realtime_delete_key(const store_key_t &key, repli_timestamp timestamp) {
    assert_thread();

    debugf("send realtime_delete_key(%.*s), %d\n", key.size, key.contents, int(bool(*stream_)));

    size_t n = sizeof(net_delete_t) + key.size;
    if (*stream_) {
        scoped_malloc<net_delete_t> msg(n);
        msg->timestamp = timestamp;
        msg->key_size = key.size;
        memcpy(msg->key, key.contents, key.size);

        if (*stream_) (*stream_)->send(msg.get());
    }
}

void backfill_sender_t::realtime_time_barrier(repli_timestamp timestamp) {
    debugf("send realtime_time_barrier(), %d\n", int(bool(*stream_)));
    assert_thread();
    net_nop_t msg;
    msg.timestamp = timestamp;
    if (*stream_) (*stream_)->send(msg);
}

}
