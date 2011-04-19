#include "master.hpp"

#include <boost/scoped_ptr.hpp>

#include "db_thread_info.hpp"
#include "logger.hpp"
#include "replication/backfill.hpp"
#include "replication/net_structs.hpp"
#include "replication/protocol.hpp"
#include "server/key_value_store.hpp"

namespace replication {

void master_t::on_tcp_listener_accept(boost::scoped_ptr<linux_tcp_conn_t>& conn) {
    // TODO: Carefully handle case where a slave is already connected.

    // Right now we uncleanly close the slave connection.  What if
    // somebody has partially written a message to it (and writes the
    // rest of the message to conn?)  That will happen, the way the
    // code is, right now.
    debugf("listener accept, destroying existing slave conn\n");
    destroy_existing_slave_conn_if_it_exists();
    debugf("making new repli_stream..\n");

    if (!get_permission_) {
        /* We just got our first slave connection (at least, the first one since the master
        was last shut down). Continue refusing to allow gets or sets, but update the message */
        get_gate_->set_message("currently reverse-backfilling from slave; data might be inconsistent.");
        set_gate_->set_message("currently reverse-backfilling from slave; data might be inconsistent.");
    }

    stream_ = new repli_stream_t(conn, this);
    stream_exists_cond_.reset();
    debugf("made repli_stream.\n");

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

    if (!get_permission_) {
        /* We just finished the reverse-backfill operation from the first slave.
        Now we can accept gets and sets. At this point we will continue accepting
        gets and sets until we are shut down, regardless of whether the slave connection
        dies again. */
        get_permission_.reset(new gated_get_store_t::open_t(get_gate_));
        set_permission_.reset(new gated_set_store_interface_t::open_t(set_gate_));
    }

    assert_thread();

    if (stream_) {
        assert_thread();

        /* So we can't shut down yet */
        streaming_cond_.reset();

        multicond_t mc; // when mc is pulsed, backfill_and_realtime_stream() will return
        interrupt_streaming_cond_.watch(&mc);
        backfill_and_realtime_stream(kvs_, since_when, this, &mc);

        /* So we can shut down */
        streaming_cond_.pulse();
    }

    /* We leave get_permission_ and set_permission_ open so that we continue accepting queries
    even now that the slave is down */
}

}  // namespace replication

