#ifndef __REPLICATION_SLAVE_STREAM_MANAGER_HPP__
#define __REPLICATION_SLAVE_STREAM_MANAGER_HPP__

#include "replication/protocol.hpp"
#include "replication/queueing_store.hpp"

namespace replication {

/* slave_stream_manager_t manages the connection to the master. Here's how it works:
The run() function constructs a slave_stream_manager_t and attaches it to a multicond_t.
It also attaches a multicond_weak_ptr_t to the multicond_t. Then it waits on the
multicond_t. If the connection to the master dies, the multicond_t will be pulsed. If
it needs to shut down the connection for some other reason (like if the slave got a
SIGINT) then it pulses the multicond, and the slave_stream_manager_t automatically closes
the connection to the master. */

struct slave_stream_manager_t :
    public message_callback_t,
    public home_thread_mixin_t,
    public multicond_t::waiter_t
{
    // Give it a connection to the master, a pointer to the store to forward changes to, and a
    // multicond. If the multicond is pulsed, it will kill the connection. If the connection dies,
    // it will pulse the multicond.
    slave_stream_manager_t(boost::scoped_ptr<tcp_conn_t> *conn, queueing_store_t *is, multicond_t *multicond);

    ~slave_stream_manager_t();

    void backfill(repli_timestamp since_when);

#ifdef REVERSE_BACKFILLING
    // Called by run() after the slave_stream_manager_t is created. Fold into constructor?
    void reverse_side_backfill(repli_timestamp since_when);
#endif

    /* message_callback_t interface */
    // These call .swap on their parameter, taking ownership of the pointee.
    void hello(net_hello_t message);
    void send(scoped_malloc<net_backfill_t>& message);
    void send(scoped_malloc<net_backfill_complete_t>& message);
    void send(scoped_malloc<net_announce_t>& message);
    void send(scoped_malloc<net_get_cas_t>& message);
    void send(stream_pair<net_sarc_t>& message);
    void send(stream_pair<net_backfill_set_t>& message);
    void send(scoped_malloc<net_incr_t>& message);
    void send(scoped_malloc<net_decr_t>& message);
    void send(stream_pair<net_append_t>& message);
    void send(stream_pair<net_prepend_t>& message);
    void send(scoped_malloc<net_delete_t>& message);
    void send(scoped_malloc<net_backfill_delete_t>& message);
    void send(scoped_malloc<net_nop_t>& message);
    void send(scoped_malloc<net_ack_t>& message);
    void conn_closed();

    // Called when the multicond is pulsed for some other reason
    void on_multicond_pulsed();

    // Our connection to the master
    repli_stream_t *stream_;

    // What to forward queries to
    queueing_store_t *internal_store_;

    // If multicond_ is pulsed, we drop our connection to the master. If the connection
    // to the master drops on its own, we pulse multicond_.
    multicond_t *multicond_;

    // True iff we are currently recieving a stream of backfilled data from the master
    bool backfilling_;

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


