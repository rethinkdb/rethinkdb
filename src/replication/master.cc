#include "master.hpp"

#include <boost/scoped_ptr.hpp>

#include "logger.hpp"
#include "replication/net_structs.hpp"
#include "replication/protocol.hpp"
#include "server/slice_dispatching_to_master.hpp"

namespace replication {

void master_t::register_dispatcher(btree_slice_dispatching_to_master_t *dispatcher) {
    dispatchers_.push_back(dispatcher);
}

void master_t::get_cas(const store_key_t& key, castime_t castime) {
    // There is something disgusting with this.  What if the thing
    // that the tmp_hold would prevent happens before we grap the
    // tmp_hold?  Right now, this can't happen because spawn_on_thread
    // won't block before we grab the tmp_hold, because we don't do
    // anything before.

    // TODO Is grabbing a tmp_hold really necessary?  I wish it wasn't.
    snag_ptr_t<master_t> tmp_hold(*this);

    if (stream_) {
        consider_nop_dispatch_and_update_latest_timestamp(castime.timestamp);

        size_t n = sizeof(net_get_cas_t) + key.size;
        scoped_malloc<net_get_cas_t> msg(n);
        msg->proposed_cas = castime.proposed_cas;
        msg->timestamp = castime.timestamp;
        msg->key_size = key.size;

        memcpy(msg->key, key.contents, key.size);

        stream_->send(msg.get());
    }
}

void master_t::sarc(const store_key_t& key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    snag_ptr_t<master_t> tmp_hold(*this);

    if (stream_) {
        consider_nop_dispatch_and_update_latest_timestamp(castime.timestamp);

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
        stream_->send(&stru, key.contents, data);
    }
    // TODO: do we delete data or does repli_stream_t delete it?
    delete data;
}

void master_t::incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, castime_t castime) {
    if (stream_) {
        consider_nop_dispatch_and_update_latest_timestamp(castime.timestamp);

        if (kind == incr_decr_INCR) {
            incr_decr_like<net_incr_t>(INCR, key, amount, castime);
        } else {
            rassert(kind == incr_decr_DECR);
            incr_decr_like<net_decr_t>(DECR, key, amount, castime);
        }
    }
}

template <class net_struct_type>
void master_t::incr_decr_like(uint8_t msgcode, const store_key_t &key, uint64_t amount, castime_t castime) {
    net_struct_type msg;
    msg.timestamp = castime.timestamp;
    msg.proposed_cas = castime.proposed_cas;
    msg.amount = amount;

    stream_->send(&msg);
}


void master_t::append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data, castime_t castime) {
    if (stream_) {
        consider_nop_dispatch_and_update_latest_timestamp(castime.timestamp);

        if (kind == append_prepend_APPEND) {
            net_append_t appendstruct;
            appendstruct.timestamp = castime.timestamp;
            appendstruct.proposed_cas = castime.proposed_cas;
            appendstruct.key_size = key.size;
            appendstruct.value_size = data->get_size();

            stream_->send(&appendstruct, key.contents, data);
        } else {
            rassert(kind == append_prepend_PREPEND);

            net_prepend_t prependstruct;
            prependstruct.timestamp = castime.timestamp;
            prependstruct.proposed_cas = castime.proposed_cas;
            prependstruct.key_size = key.size;
            prependstruct.value_size = data->get_size();

            stream_->send(&prependstruct, key.contents, data);
        }
    }
    delete data;
}

void master_t::delete_key(const store_key_t &key, repli_timestamp timestamp) {
    size_t n = sizeof(net_delete_t) + key.size;
    if (stream_) {
        consider_nop_dispatch_and_update_latest_timestamp(timestamp);

        scoped_malloc<net_delete_t> msg(n);
        msg->timestamp = timestamp;
        msg->key_size = key.size;
        memcpy(msg->key, key.contents, key.size);

        stream_->send(msg.get());
    }
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
        debugf("made repli_stream.\n");
    }
    // TODO when sending/receiving hello handshake, use database magic
    // to handle case where slave is already connected.

    // TODO receive hello handshake before sending other messages.
}

void master_t::destroy_existing_slave_conn_if_it_exists() {
    if (stream_) {
        stream_->co_shutdown();
    }

    stream_ = NULL;
}

void master_t::consider_nop_dispatch_and_update_latest_timestamp(repli_timestamp timestamp) {
    rassert(timestamp.time != repli_timestamp::invalid.time);

    if (timestamp.time <= latest_timestamp_.time) return;

    latest_timestamp_ = timestamp;
    coro_t::spawn(boost::bind(&master_t::do_nop_rebound, this, timestamp));
}

void master_t::do_nop_rebound(repli_timestamp t) {
    cond_t cond;
    int counter = dispatchers_.size();
    for (std::vector<btree_slice_dispatching_to_master_t *>::iterator p = dispatchers_.begin(), e = dispatchers_.end();
         p != e;
         ++p) {
        coro_t::spawn(boost::bind(&btree_slice_dispatching_to_master_t::nop_back_on_masters_thread, *p, t, &cond, &counter));
    }

    cond.wait();

    net_nop_t msg;
    msg.timestamp = t;
    stream_->send(msg);
}


}  // namespace replication

