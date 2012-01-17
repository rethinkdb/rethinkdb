#include "master.hpp"

#include <boost/scoped_ptr.hpp>

#include "db_thread_info.hpp"
#include "logger.hpp"
#include "replication/backfill.hpp"
#include "replication/net_structs.hpp"
#include "replication/protocol.hpp"
#include "server/key_value_store.hpp"

namespace replication {

void master_t::on_conn(boost::scoped_ptr<linux_tcp_conn_t>& conn) {
    mutex_acquisition_t ak(&stream_setup_teardown_);

    // Note: As destroy_existing_slave_conn_if_it_exists() acquires
    // stream_setup_teardown_ now, we would have to be careful in case
    // we wanted to terminate existing slave connections.  Instead we
    // terminate the incoming connection.
    if (stream_) { 
        logWRN("Rejecting slave connection because I already have one.\n");
        return;
    }
    if (!streaming_cond_.get_signal() || !streaming_cond_.get_signal()->is_pulsed()) {
        logWRN("Rejecting slave connection because the previous one has not cleaned up yet.\n");
        return;
    }

    if (!get_permission_) {
        /* We just got our first slave connection (at least, the first one since the master
        was last shut down). Continue refusing to allow gets or sets, but update the message */
        logINF("Slave connected; receiving updates from slave...\n");
        get_gate_->set_message("currently reverse-backfilling from slave; data might be inconsistent.");
        set_gate_->set_message("currently reverse-backfilling from slave; data might be inconsistent.");
    } else {
        logINF("Slave connected; sending updates to slave...\n");
    }

    stream_ = new repli_stream_t(conn, this, replication_config_.heartbeat_timeout);
    stream_exists_cond_.reset();

    /* Send the slave our database creation timestamp so it knows which master it's connected to. */
    net_introduce_t introduction;
    introduction.database_creation_timestamp = kvs_->get_creation_timestamp();

    // I guess this is okay.
    sequence_group_t local_seq_group;
    introduction.other_id = kvs_->get_replication_slave_id(&local_seq_group);
    stream_->send(&introduction);

    // TODO: When sending/receiving hello handshake, use database
    // magic to handle case where slave is already connected.

    // TODO: Receive hello handshake before sending other messages.
}

void master_t::destroy_existing_slave_conn_if_it_exists() {
    assert_thread();
    // We acquire the stream_setup_teardown_ mutex to make sure that we don't interfere
    // with half-opened or half-closed connections
    boost::scoped_ptr<mutex_acquisition_t> ak(new mutex_acquisition_t(&stream_setup_teardown_));
    if (stream_) {
        // We have to release the mutex, otherwise the following would dead-lock
        ak.reset();

        stream_->shutdown();   // Will cause conn_closed() to happen
        stream_exists_cond_.get_signal()->wait();   // Waits until conn_closed() happens
    }
    // Waits until a running backfill is over. This is outside of the "if" statement because
    // there might still be a running backfill even when there is no stream. (That was what
    // caused the second half of issue #326.)
    streaming_cond_.get_signal()->wait();
    rassert(stream_ == NULL);
}

void master_t::do_backfill_and_realtime_stream(repli_timestamp since_when) {

#ifndef NDEBUG
    rassert(!master_t::inside_backfill_done_or_backfill);
    master_t::inside_backfill_done_or_backfill = true;
#endif

    // Because opening the gate is blocking, there is a race where while we are opening
    // the gates, the connection is closed. This is ok, but if before we actually
    // get down to check stream_, a new connection could have been established.
    // In that case we can end up in weird situations.
    // To make sure this doesn't happen, we reset the streaming_cond during this
    // whole process, so no new slave can connect.

    /* So we can't shut down yet */
    streaming_cond_.reset();

    bool opened = false;
    if (!get_permission_ && stream_) {
        /* We just finished the reverse-backfill operation from the first slave.
        Now we can accept gets and sets. At this point we will continue accepting
        gets and sets until we are shut down, regardless of whether the slave connection
        dies again. */

        logINF("Now accepting operations normally.\n");
        opened = true;
        get_permission_.reset(new gated_get_store_t::open_t(get_gate_));
        set_permission_.reset(new gated_set_store_interface_t::open_t(set_gate_));
    }

    assert_thread();

    if (stream_) {

        cond_t cond; // when cond is pulsed, backfill_and_realtime_stream() will return
        interrupt_streaming_cond_.watch(&cond);
        backfill_and_realtime_stream(kvs_, since_when, this, &cond);

        debugf("backfill_and_realtime_stream() returned.\n");
    } else if (opened) {
        // This might be useful for debugging #366, not sure if it's a useful message
        // in general...
        debugf("The slave disconnected right after we started accepting operations.\n");
    }

    /* So we can shut down */
    streaming_cond_.pulse();

    /* We leave get_permission_ and set_permission_ open so that we continue accepting queries
    even now that the slave is down */

#ifndef NDEBUG
    master_t::inside_backfill_done_or_backfill = false;
#endif

}

#ifndef NDEBUG
bool master_t::inside_backfill_done_or_backfill = false;
#endif

}  // namespace replication

