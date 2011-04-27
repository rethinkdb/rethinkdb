#ifndef __REPLICATION_SLAVE_STREAM_MANAGER_HPP__
#define __REPLICATION_SLAVE_STREAM_MANAGER_HPP__

#include "replication/protocol.hpp"
#include "replication/backfill_receiver.hpp"
#include "replication/backfill_in.hpp"
#include "server/key_value_store.hpp"

namespace replication {

/* slave_stream_manager_t manages the connection to the master. Here's how it works:
The run() function constructs a slave_stream_manager_t and attaches it to a multicond_t.
It also attaches a multicond_weak_ptr_t to the multicond_t. Then it waits on the
multicond_t. If the connection to the master dies, the multicond_t will be pulsed. If
it needs to shut down the connection for some other reason (like if the slave got a
SIGINT) then it pulses the multicond, and the slave_stream_manager_t automatically closes
the connection to the master. */

struct slave_stream_manager_t :
    public backfill_receiver_t,
    public home_thread_mixin_t,
    public multicond_t::waiter_t
{
    // Give it a connection to the master, a pointer to the store to forward changes to, and a
    // multicond. If the multicond is pulsed, it will kill the connection. If the connection dies,
    // it will pulse the multicond.
    slave_stream_manager_t(boost::scoped_ptr<tcp_conn_t> *conn, btree_key_value_store_t *kvs, multicond_t *multicond);

    ~slave_stream_manager_t();

    /* backfill() returns when the backfill is over, but continues streaming realtime updates even
    after it has returned */
    void backfill(repli_timestamp since_when);

    // Called by run() after the slave_stream_manager_t is created. Fold into constructor?
    void reverse_side_backfill(repli_timestamp since_when);

    /* message_callback_t interface */
    // These call .swap on their parameter, taking ownership of the pointee.
    void hello(net_hello_t message);

    void send(scoped_malloc<net_backfill_t>& message);

    // Overrides backfill_receiver_t::send(scoped_malloc<net_backfill_complete_t>&)
    void send(scoped_malloc<net_backfill_complete_t>& message) {
        backfill_receiver_t::send(message);
        backfill_done_cond_.pulse_if_non_null();
    }
    multicond_weak_ptr_t backfill_done_cond_;

    void conn_closed();

    // Called when the multicond is pulsed for some other reason
    void on_multicond_pulsed();

    // Our connection to the master
    repli_stream_t *stream_;

    // If multicond_ is pulsed, we drop our connection to the master. If the connection
    // to the master drops on its own, we pulse multicond_.
    multicond_t *multicond_;

    btree_key_value_store_t *kvs_;

    // For backfilling
    backfill_storer_t backfill_storer_;

    // When conn_closed() is called, we consult interrupted_by_external_event_ to determine
    // if this is a spontaneous loss of connectivity or if it's due to an intentional
    // shutdown.
    bool interrupted_by_external_event_;

    // In our destructor, we block on shutdown_cond_ to make sure that the repli_stream_t
    // has actually completed its shutdown process.
    multicond_t shutdown_cond_;
};

}

#endif /* __REPLICATION_SLAVE_STREAM_MANAGER_HPP__ */


