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

