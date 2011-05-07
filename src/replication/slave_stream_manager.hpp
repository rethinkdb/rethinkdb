#ifndef __REPLICATION_SLAVE_STREAM_MANAGER_HPP__
#define __REPLICATION_SLAVE_STREAM_MANAGER_HPP__

#include "replication/protocol.hpp"
#include "replication/backfill_receiver.hpp"
#include "replication/backfill_in.hpp"
#include "server/key_value_store.hpp"

namespace replication {

/* slave_stream_manager_t manages the connection to the master. Here's how it works:
The run() function constructs a slave_stream_manager_t and attaches it to a cond_t.
It also attaches a cond_weak_ptr_t to the cond_t. Then it waits on the
cond_t. If the connection to the master dies, the cond_t will be pulsed. If
it needs to shut down the connection for some other reason (like if the slave got a
SIGINT) then it pulses the cond, and the slave_stream_manager_t automatically closes
the connection to the master. */

struct slave_stream_manager_t :
    public backfill_receiver_t,
    public home_thread_mixin_t,
    public cond_t::waiter_t
{
    // Give it a connection to the master, a pointer to the store to forward changes to, and a
    // cond. If the cond is pulsed, it will kill the connection. If the connection dies,
    // it will pulse the cond.
    slave_stream_manager_t(boost::scoped_ptr<tcp_conn_t> *conn, btree_key_value_store_t *kvs, cond_t *cond);

    ~slave_stream_manager_t();

    /* backfill() returns when the backfill is over, but continues streaming realtime updates even
    after it has returned */
    void backfill(repli_timestamp since_when);

    // Called by run() after the slave_stream_manager_t is created. Fold into constructor?
    void reverse_side_backfill(repli_timestamp since_when);

    /* message_callback_t interface */
    // These call .swap on their parameter, taking ownership of the pointee.
    void hello(net_hello_t message);

    void send(scoped_malloc<net_introduce_t>& message) {

        uint32_t old_master = kvs_->get_replication_master_id();

        if (old_master == 0) {
            /* This is our first-ever time running; the database files are completely fresh. */
            if (message->other_id == 0) {
                logINF("We are the first slave to connect to this master.\n");
            } else {
                logWRN("The slave that was previously attached to this master will be forgotten; "
                    "you will not be able to reconnect it later. The master will remember this "
                    "slave instead.\n");
            }

            /* Make the master remember us */
            net_introduce_t introduce;
            introduce.database_creation_timestamp = kvs_->get_creation_timestamp();
            introduce.other_id = 0;
            if (stream_) stream_->send(&introduce);

            /* Remember the master */
            kvs_->set_replication_master_id(message->database_creation_timestamp);

        } else if (old_master == NOT_A_SLAVE) {
            /* We have run before, but in master-mode or non-replicated mode. There might be
            leftover data in the database, which could lead to undefined behavior. */
            fail_due_to_user_error("It is illegal to turn an existing database into a slave. You "
                "must run a slave on a fresh data file or on a data file from a previous slave of "
                "the same master.");

        } else if (old_master != message->database_creation_timestamp) {
            /* We have run before in slave mode, but as a slave of a different master. */
            fail_due_to_user_error("The master that we are currently a slave of is a different "
                "database than the one we were a slave of before. That doesn't work.");

        } else if (message->other_id != kvs_->get_creation_timestamp()) {
            /* We were a slave of this master at some point in the past, but it has since
            forgotten about us. It might have changes on it from some other slave and we can't
            safely backfill because backfilled deletions don't go in the delete queue. */
            fail_due_to_user_error("Although we have connected to this master in the past, it has "
                "forgotten about us because another slave was connected to it more recently. Only "
                "newly-created slaves can displace existing slaves; if you want this machine to "
                "act as a slave again, you must wipe the data files and then reconnect to the "
                "master.");
        }
    }

    void send(scoped_malloc<net_backfill_t>& message);

    // Overrides backfill_receiver_t::send(scoped_malloc<net_backfill_complete_t>&)
    void send(scoped_malloc<net_backfill_complete_t>& message) {
        backfill_receiver_t::send(message);
        backfill_done_cond_.pulse_if_non_null();
    }
    cond_weak_ptr_t backfill_done_cond_;

    void conn_closed();

    // Called when the cond is pulsed for some other reason
    void on_signal_pulsed();

    // Our connection to the master
    repli_stream_t *stream_;

    // If cond_ is pulsed, we drop our connection to the master. If the connection
    // to the master drops on its own, we pulse cond_.
    cond_t *cond_;

    btree_key_value_store_t *kvs_;

    // For backfilling
    backfill_storer_t backfill_storer_;

    // When conn_closed() is called, we consult interrupted_by_external_event_ to determine
    // if this is a spontaneous loss of connectivity or if it's due to an intentional
    // shutdown.
    bool interrupted_by_external_event_;

    // In our destructor, we block on shutdown_cond_ to make sure that the repli_stream_t
    // has actually completed its shutdown process.
    cond_t shutdown_cond_;
};

}

#endif /* __REPLICATION_SLAVE_STREAM_MANAGER_HPP__ */


