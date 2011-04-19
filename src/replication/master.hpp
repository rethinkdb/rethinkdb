#ifndef __REPLICATION_MASTER_HPP__
#define __REPLICATION_MASTER_HPP__

#include "store.hpp"
#include "arch/arch.hpp"
#include "gated_store.hpp"
#include "replication/backfill_out.hpp"
#include "replication/backfill_sender.hpp"
#include "replication/backfill_receiver.hpp"
#include "replication/backfill_in.hpp"
#include "concurrency/mutex.hpp"
#include "containers/thick_list.hpp"
#include "replication/net_structs.hpp"
#include "replication/protocol.hpp"
#include "replication/queueing_store.hpp"

class btree_key_value_store_t;

namespace replication {

// master_t is a class that manages a connection to a slave.

class master_t :
    public linux_tcp_listener_callback_t,
    public backfill_sender_t,
    public backfill_receiver_t {
public:
    master_t(int port, btree_key_value_store_t *kv_store, gated_get_store_t *get_gate, gated_set_store_interface_t *set_gate) :
        backfill_sender_t(&stream_),
        backfill_receiver_t(&backfill_storer_),
        stream_(NULL),
        listener_port_(port),
        kvs_(kv_store),
        get_gate_(get_gate),
        set_gate_(set_gate),
        backfill_storer_(kv_store)
    {
        listener_.reset(new tcp_listener_t(listener_port_));
        listener_->set_callback(this);

        // Because stream_ is initially NULL
        stream_exists_cond_.pulse();

        // Because there is initially no backfill operation
        streaming_cond_.pulse();

        /* Initially, we don't allow either gets or sets, because no slave has connected yet so
        we don't know how out-of-date our data might be. */
        get_gate_->set_message("haven't gotten initial slave connection yet; data might be out of date.");
        set_gate_->set_message("haven't gotten initial slave connection yet; data might be out of date.");
    }

    ~master_t() {

        // Stop listening for new connections
        listener_.reset();

        destroy_existing_slave_conn_if_it_exists();
    }

    bool has_slave() { return stream_ != NULL; }

    // Listener callback functions
    void on_tcp_listener_accept(boost::scoped_ptr<linux_tcp_conn_t>& conn);

    // TODO: kill slave connection instead of crashing server when slave sends garbage.
    void hello(UNUSED net_hello_t message) { debugf("Received hello from slave.\n"); }
    void send(UNUSED scoped_malloc<net_backfill_t>& message) {
        coro_t::spawn_now(boost::bind(&master_t::do_backfill_and_realtime_stream, this, message->timestamp));
    }
    void send(UNUSED scoped_malloc<net_announce_t>& message) { guarantee(false, "slave sent announce"); }
    void send(UNUSED scoped_malloc<net_ack_t>& message) { crash("ack is obsolete"); }
    void conn_closed() {
        assert_thread();

        /* The stream destructor may block, so we set stream_ to NULL before calling the stream
        destructor. */
        rassert(stream_);
        repli_stream_t *stream_copy = stream_;
        stream_ = NULL;
        delete stream_copy;

        stream_exists_cond_.pulse();    // If anything was waiting for stream to close, signal it
        interrupt_streaming_cond_.pulse_if_non_null();   // Will interrupt any running backfill/stream operation
    }

    void do_backfill_and_realtime_stream(repli_timestamp since_when);

private:

    void destroy_existing_slave_conn_if_it_exists();

    // The stream to the slave, or NULL if there is no slave connected.
    repli_stream_t *stream_;

    int listener_port_;
    // Listens for incoming slave connections.
    boost::scoped_ptr<tcp_listener_t> listener_;

    // The key value store.
    btree_key_value_store_t *kvs_;

    // Pointers to the gates we use to allow/disallow gets and sets
    gated_get_store_t *get_gate_;
    gated_set_store_interface_t *set_gate_;
    boost::scoped_ptr<gated_get_store_t::open_t> get_permission_;
    boost::scoped_ptr<gated_set_store_interface_t::open_t> set_permission_;

    // For reverse-backfilling;
    backfill_storer_t backfill_storer_;

    // This is unpulsed iff stream_ is non-NULL.
    cond_t stream_exists_cond_; 

    // This is unpulsed iff there is not a running backfill/stream operation
    cond_t streaming_cond_;

    // Pulse this to interrupt a running backfill/realtime stream operation
    multicond_weak_ptr_t interrupt_streaming_cond_;

    DISABLE_COPYING(master_t);
};

}  // namespace replication

#endif  // __REPLICATION_MASTER_HPP__
