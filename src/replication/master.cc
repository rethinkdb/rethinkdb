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
    snag_ptr_t<master_t> tmp_hold(*this);

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
void master_t::incr_decr_like(UNUSED uint8_t msgcode, UNUSED const store_key_t &key, uint64_t amount, castime_t castime) {
    // TODO: We aren't using the parameter key.  How did we do this?
    // TODO: We aren't using the parameter msgcode.  This is obviously broken!

    net_struct_type msg;
    msg.timestamp = castime.timestamp;
    msg.proposed_cas = castime.proposed_cas;
    msg.amount = amount;

    stream_->send(&msg);
}


void master_t::append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data, castime_t castime) {
    snag_ptr_t<master_t> tmp_hold(*this);

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
    snag_ptr_t<master_t> tmp_hold(*this);

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

void nop_timer_trigger(void *master_) {
    master_t *master = reinterpret_cast<master_t *>(master_);

    master->consider_nop_dispatch_and_update_latest_timestamp(current_time());
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
        next_timestamp_nop_timer_ = timer_handler_->add_timer_internal(1100, nop_timer_trigger, this, true);
        debugf("made repli_stream.\n");
    }
    // TODO when sending/receiving hello handshake, use database magic
    // to handle case where slave is already connected.

    // TODO receive hello handshake before sending other messages.
}

void master_t::destroy_existing_slave_conn_if_it_exists() {
    assert_thread();
    if (stream_) {
        stream_->co_shutdown();
        cancel_timer(next_timestamp_nop_timer_);
        next_timestamp_nop_timer_ = NULL;
    }

    stream_ = NULL;
}

void master_t::consider_nop_dispatch_and_update_latest_timestamp(repli_timestamp timestamp) {
    assert_thread();
    rassert(timestamp.time != repli_timestamp::invalid.time);

    if (timestamp.time > latest_timestamp_.time) {
        latest_timestamp_ = timestamp;
        coro_t::spawn(boost::bind(&master_t::do_nop_rebound, this, timestamp));
    }

    timer_handler_->cancel_timer(next_timestamp_nop_timer_);
    next_timestamp_nop_timer_ = timer_handler_->add_timer_internal(1100, nop_timer_trigger, this, true);
}

void master_t::do_nop_rebound(repli_timestamp t) {
    assert_thread();
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

struct do_backfill_cb : public backfill_callback_t {
    int count;
    master_t *master;
    cond_t *for_when_done;

    void deletion_key(UNUSED const store_key_t *key) {
        coro_t::spawn_on_thread(master->home_thread, boost::bind(&master_t::send_deletion_key_to_slave, master, *key));
    }
    void done_deletion_keys() {
        coro_t::spawn_on_thread(master->home_thread, boost::bind(&do_backfill_cb::do_done, this));
    }

    void on_keyvalue(backfill_atom_t atom) {
        coro_t::spawn_on_thread(master->home_thread, boost::bind(&master_t::send_backfill_atom_to_slave, master, atom));
    }
    void done() {
        coro_t::spawn_on_thread(master->home_thread, boost::bind(&do_backfill_cb::do_done, this));
    }

    void do_done() {
        rassert(get_thread_id() == master->home_thread);

        count = count - 1;
        debugf("do_done, decrementing count to %d\n", count);
        if (0 == count) {
            for_when_done->pulse();
        }
    }
};

void master_t::do_backfill(repli_timestamp since_when) {
    // TODO: Right now, this tmp_hold means that we will wait until we
    // have finished backfilling before shutting down.  This is
    // undesireable behavior.  (Of course, if we simply remove the
    // tmp_hold, we have an unclean shutdown process.  So we need to
    // properly implement the shutting down of master.)

    snag_ptr_t<master_t> tmp_hold(*this);
    assert_thread();
    debugf("Somebody called do_backfill(%u)\n", since_when.time);

    int n = dispatchers_.size();
    cond_t done_cond;

    do_backfill_cb cb;
    debugf("do_backfill_cb, count = %d\n", n);
    cb.count = 2*n;
    cb.master = this;
    cb.for_when_done = &done_cond;

    for (int i = 0; i < n; ++i) {
        dispatchers_[i]->spawn_backfill(since_when, &cb);
    }

    debugf("Done spawning, now waiting for done_cond...\n");
    done_cond.wait();
    debugf("Done backfill.\n");
    if (stream_) {
        debugf("Sending backfill_complete...\n");
        net_backfill_complete_t msg;
        memset(msg.ignore, 0, sizeof(msg.ignore));
        stream_->send(&msg);
        debugf("Sent backfill_complete.\n");
    } else {
        debugf("Not sending backfill...\n");
    }
}

void master_t::send_backfill_atom_to_slave(backfill_atom_t atom) {
    snag_ptr_t<master_t> tmp_hold(*this);

    data_provider_t *data = atom.value.release();

    if (stream_) {
        net_backfill_set_t msg;
        msg.timestamp = atom.recency;
        msg.flags = atom.flags;
        msg.exptime = atom.exptime;
        msg.cas_or_zero = atom.cas_or_zero;
        msg.key_size = atom.key.size;
        msg.value_size = data->get_size();
        stream_->send(&msg, atom.key.contents, data);
    }

    // TODO: do we delete data or does repli_stream_t delete it?
    delete data;
}

void master_t::send_deletion_key_to_slave(store_key_t key) {
    snag_ptr_t<master_t> tmp_hold(*this);

    size_t n = sizeof(net_backfill_delete_t) + key.size;
    if (stream_) {
        scoped_malloc<net_backfill_delete_t> msg(n);
        msg->key_size = key.size;
        memcpy(msg->key, key.contents, key.size);

        stream_->send(msg.get());
    }
}


}  // namespace replication

