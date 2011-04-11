#ifndef __REPLICATION_MASTER_HPP__
#define __REPLICATION_MASTER_HPP__

#include "store.hpp"
#include "arch/arch.hpp"
#include "replication/backfill.hpp"
#include "replication/backfill_sender.hpp"
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
    public message_callback_t,
    public backfill_sender_t {
public:
    master_t(int port) :
        backfill_sender_t(&stream_),
        stream_(NULL),
        listener_port_(port)
    {

        // Because stream_ is initially NULL
        stream_exists_cond_.pulse();

        // Because there is initially no backfill operation
        streaming_cond_.pulse();
    }

    ~master_t() {

        // Stop listening for new connections
        listener_.reset();

        destroy_existing_slave_conn_if_it_exists();
    }

    // The master does not turn on its listener until this function is called.
    void register_key_value_store(btree_key_value_store_t *kv_store);

    bool has_slave() { return stream_ != NULL; }

    // Listener callback functions
    void on_tcp_listener_accept(boost::scoped_ptr<linux_tcp_conn_t>& conn);

    // TODO: kill slave connection instead of crashing server when slave sends garbage.
    void hello(UNUSED net_hello_t message) { debugf("Received hello from slave.\n"); }
    void send(UNUSED scoped_malloc<net_backfill_t>& message) {
        coro_t::spawn_now(boost::bind(&master_t::do_backfill_and_realtime_stream, this, message->timestamp));
    }
    void send(UNUSED scoped_malloc<net_backfill_complete_t>& message) {
#ifdef REVERSE_BACKFILLING
        // TODO: What about time_barrier, which the slave side does?

        queue_store_->backfill_complete();
        debugf("Slave sent BACKFILL_COMPLETE.\n");
#else
        crash("reverse backfilling is disabled");
        (void)message;
#endif
    }
    void send(UNUSED scoped_malloc<net_announce_t>& message) { guarantee(false, "slave sent announce"); }
    void send(UNUSED scoped_malloc<net_get_cas_t>& message) { guarantee(false, "slave sent get_cas"); }
    void send(UNUSED stream_pair<net_sarc_t>& message) { guarantee(false, "slave sent sarc"); }
    void send(stream_pair<net_backfill_set_t>& msg) {
        // TODO: this is duplicate code.

        sarc_mutation_t mut;
        mut.key.assign(msg->key_size, msg->keyvalue);
        mut.data = msg.stream;
        mut.flags = msg->flags;
        mut.exptime = msg->exptime;
        mut.add_policy = add_policy_yes;
        mut.replace_policy = replace_policy_yes;
        mut.old_cas = NO_CAS_SUPPLIED;

        // TODO: We need this operation to force the cas to be set.
        queue_store_->backfill_handover(new mutation_t(mut), castime_t(msg->cas_or_zero, msg->timestamp));
    }

    void send(UNUSED scoped_malloc<net_incr_t>& message) { guarantee(false, "slave sent incr"); }
    void send(UNUSED scoped_malloc<net_decr_t>& message) { guarantee(false, "slave sent decr"); }
    void send(UNUSED stream_pair<net_append_t>& message) { guarantee(false, "slave sent append"); }
    void send(UNUSED stream_pair<net_prepend_t>& message) { guarantee(false, "slave sent prepend"); }
    void send(UNUSED scoped_malloc<net_delete_t>& message) { guarantee(false, "slave sent delete"); }
    void send(scoped_malloc<net_backfill_delete_t>& msg) {
        // TODO: this is duplicate code.

        delete_mutation_t mut;
        mut.key.assign(msg->key_size, msg->key);
        // NO_CAS_SUPPLIED is not used in any way for deletions, and the
        // timestamp is part of the "->change" interface in a way not
        // relevant to slaves -- it's used when putting deletions into the
        // delete queue.
        queue_store_->backfill_handover(new mutation_t(mut), castime_t(NO_CAS_SUPPLIED, repli_timestamp::invalid));
    }
    void send(UNUSED scoped_malloc<net_nop_t>& message) { guarantee(false, "slave sent nop"); }
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
    boost::scoped_ptr<queueing_store_t> queue_store_;

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
