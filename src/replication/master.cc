#include "master.hpp"

#include <boost/scoped_ptr.hpp>

#include "db_thread_info.hpp"
#include "logger.hpp"
#include "replication/backfill.hpp"
#include "replication/net_structs.hpp"
#include "replication/protocol.hpp"
#include "server/key_value_store.hpp"

namespace replication {

void master_t::register_key_value_store(btree_key_value_store_t *store) {
    on_thread_t th(home_thread);
    rassert(queue_store_ == NULL);
    queue_store_.reset(new queueing_store_t(store));
    rassert(!listener_);
    listener_.reset(new tcp_listener_t(listener_port_));
    listener_->set_callback(this);
}

void master_t::backfill_deletion(store_key_t key) {
    size_t n = sizeof(net_backfill_delete_t) + key.size;
    if (stream_) {
        scoped_malloc<net_backfill_delete_t> msg(n);
        msg->padding = 0;
        msg->key_size = key.size;
        memcpy(msg->key, key.contents, key.size);

        stream_->send(msg.get());
    }
}

void master_t::backfill_set(backfill_atom_t atom) {
    if (stream_) {
        net_backfill_set_t msg;
        msg.timestamp = atom.recency;
        msg.flags = atom.flags;
        msg.exptime = atom.exptime;
        msg.cas_or_zero = atom.cas_or_zero;
        msg.key_size = atom.key.size;
        msg.value_size = atom.value->get_size();
        stream_->send(&msg, atom.key.contents, atom.value);
    }
}

void master_t::backfill_done(repli_timestamp_t timestamp_when_backfill_began) {
    net_backfill_complete_t msg;
    msg.time_barrier_timestamp = timestamp_when_backfill_began;
    if (stream_) stream_->send(&msg);
}

void master_t::realtime_get_cas(const store_key_t& key, castime_t castime) {
    // There is something disgusting with this.  What if the thing
    // that the tmp_hold would prevent happens before we grap the
    // tmp_hold?  Right now, this can't happen because spawn_on_thread
    // won't block before we grab the tmp_hold, because we don't do
    // anything before.

    // TODO Is grabbing a tmp_hold really necessary?  I wish it wasn't.
    snag_ptr_t<master_t> tmp_hold(this);
    assert_thread();

    if (stream_) {
        size_t n = sizeof(net_get_cas_t) + key.size;
        scoped_malloc<net_get_cas_t> msg(n);
        msg->proposed_cas = castime.proposed_cas;
        msg->timestamp = castime.timestamp;
        msg->key_size = key.size;

        memcpy(msg->key, key.contents, key.size);

        if (stream_) stream_->send(msg.get());
    }
}

void master_t::realtime_sarc(const store_key_t& key, unique_ptr_t<data_provider_t> data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    snag_ptr_t<master_t> tmp_hold(this);
    assert_thread();

    if (stream_) {
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
        if (stream_) stream_->send(&stru, key.contents, data);
    }
}

void master_t::realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, castime_t castime) {
    snag_ptr_t<master_t> tmp_hold(this);
    assert_thread();

    if (stream_) {
        if (kind == incr_decr_INCR) {
            incr_decr_like<net_incr_t>(key, amount, castime);
        } else {
            rassert(kind == incr_decr_DECR);
            incr_decr_like<net_decr_t>(key, amount, castime);
        }
    }
}

template <class net_struct_type>
void master_t::incr_decr_like(const store_key_t &key, uint64_t amount, castime_t castime) {
    size_t n = sizeof(net_struct_type) + key.size;

    assert_thread();

    scoped_malloc<net_struct_type> msg(n);
    msg->timestamp = castime.timestamp;
    msg->proposed_cas = castime.proposed_cas;
    msg->amount = amount;
    msg->key_size = key.size;
    memcpy(msg->key, key.contents, key.size);

    if (stream_) stream_->send(msg.get());
}

void master_t::realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key, unique_ptr_t<data_provider_t> data, castime_t castime) {
    snag_ptr_t<master_t> tmp_hold(this);
    assert_thread();

    if (stream_) {
        if (kind == append_prepend_APPEND) {
            net_append_t appendstruct;
            appendstruct.timestamp = castime.timestamp;
            appendstruct.proposed_cas = castime.proposed_cas;
            appendstruct.key_size = key.size;
            appendstruct.value_size = data->get_size();

            if (stream_) stream_->send(&appendstruct, key.contents, data);
        } else {
            rassert(kind == append_prepend_PREPEND);

            net_prepend_t prependstruct;
            prependstruct.timestamp = castime.timestamp;
            prependstruct.proposed_cas = castime.proposed_cas;
            prependstruct.key_size = key.size;
            prependstruct.value_size = data->get_size();

            if (stream_) stream_->send(&prependstruct, key.contents, data);
        }
    }
}

void master_t::realtime_delete_key(const store_key_t &key, repli_timestamp timestamp) {
    snag_ptr_t<master_t> tmp_hold(this);
    assert_thread();

    size_t n = sizeof(net_delete_t) + key.size;
    if (stream_) {
        scoped_malloc<net_delete_t> msg(n);
        msg->timestamp = timestamp;
        msg->key_size = key.size;
        memcpy(msg->key, key.contents, key.size);

        if (stream_) stream_->send(msg.get());
    }
}

void master_t::realtime_time_barrier(repli_timestamp timestamp) {
    assert_thread();
    net_nop_t msg;
    msg.timestamp = timestamp;
    if (stream_) stream_->send(msg);
}

void master_t::on_tcp_listener_accept(boost::scoped_ptr<linux_tcp_conn_t>& conn) {
    // TODO: Carefully handle case where a slave is already connected.

    // Right now we uncleanly close the slave connection.  What if
    // somebody has partially written a message to it (and writes the
    // rest of the message to conn?)  That will happen, the way the
    // code is, right now.
    {
        debugf("listener accept, destroying existing slave conn\n");
        destroy_existing_slave_conn_if_it_exists();
        debugf("making new repli_stream..\n");
        stream_ = new repli_stream_t(conn, this);
        stream_exists_cond_.reset();
        debugf("made repli_stream.\n");
    }
    // TODO when sending/receiving hello handshake, use database magic
    // to handle case where slave is already connected.

    // TODO receive hello handshake before sending other messages.
}

void master_t::destroy_existing_slave_conn_if_it_exists() {
    assert_thread();
    if (stream_) {
        stream_->shutdown();   // Will cause conn_closed() to happen
        stream_exists_cond_.wait();   // Waits until conn_closed() happens
        streaming_cond_.wait();   // Waits until a running backfill is over
    }
    rassert(stream_ == NULL);
}

void master_t::do_backfill_and_realtime_stream(repli_timestamp since_when) {

    assert_thread();

    if (stream_) {
        assert_thread();

        /* So we can't shut down yet */
        streaming_cond_.reset();

        multicond_t mc; // when mc is pulsed, backfill_and_realtime_stream() will return
        interrupt_streaming_cond_.watch(&mc);
        backfill_and_realtime_stream(queue_store_->inner(), since_when, this, &mc);

        /* So we can shut down */
        streaming_cond_.pulse();
    }
}

}  // namespace replication

