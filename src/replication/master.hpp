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
#include "replication/net_structs.hpp"
#include "replication/protocol.hpp"

class btree_key_value_store_t;

namespace replication {

// master_t is a class that manages a connection to a slave.

class master_t :
    public backfill_sender_t,
    public backfill_receiver_t {
public:
    master_t(sequence_group_t *replication_seq_group, int port, btree_key_value_store_t *kv_store, replication_config_t replication_config, gated_get_store_t *get_gate, gated_set_store_interface_t *set_gate, backfill_receiver_order_source_t *master_order_source) :
        backfill_sender_t(&stream_),
        backfill_receiver_t(&backfill_storer_, master_order_source),
        stream_(NULL),
        seq_group_(replication_seq_group),
        listener_port_(port),
        kvs_(kv_store),
        replication_config_(replication_config),
        get_gate_(get_gate),
        set_gate_(set_gate),
        backfill_storer_(replication_seq_group, kv_store),
        dont_wait_for_slave_control(this)
    {
        logINF("Waiting for initial slave to connect on port %d...\n", listener_port_);
        logINF("If no slave was connected when the master last shut down, send "
            "\"rethinkdb dont-wait-for-slave\" to the server over telnet to go ahead "
            "without waiting for the slave to connect.\n");
        listener_.reset(new tcp_listener_t(listener_port_, boost::bind(&master_t::on_conn, this, _1)));

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

        // Drain operations. It isn't really necessary to drain gets here, but we must drain sets
        // before we destroy the slave connection so that the user can be guaranteed that all
        // operations will make it to the slave after sending SIGINT.
        set_gate_->set_message("server is shutting down.");
        get_gate_->set_message("server is shutting down.");
        set_permission_.reset();
        get_permission_.reset();

        destroy_existing_slave_conn_if_it_exists();
    }

    bool has_slave() { return stream_ != NULL; }

    // Listener callback functions
    void on_conn(boost::scoped_ptr<tcp_conn_t>& conn);

    void hello(UNUSED net_hello_t message) { debugf("Received hello from slave.\n"); }

    void send(scoped_malloc<net_introduce_t>& message) {
        uint32_t previous_slave = kvs_->get_replication_slave_id(seq_group_);
        if (previous_slave != 0) {
            rassert(message->database_creation_timestamp != previous_slave);
            logWRN("The slave that was previously associated with this master is now being "
                "forgotten; you will not be able to reconnect it later.\n");
        }
        kvs_->set_replication_slave_id(seq_group_, message->database_creation_timestamp);
    }

    void send(scoped_malloc<net_backfill_t>& message) {
        coro_t::spawn_now(boost::bind(&master_t::do_backfill_and_realtime_stream, this, message->timestamp));
    }

    void send(scoped_malloc<net_timebarrier_t>& message) {
        timebarrier_helper(*message);
    }

    void conn_closed() {
        logINF("Connection to slave was closed.\n");

        assert_thread();
        mutex_acquisition_t ak(&stream_setup_teardown_);

        /* The stream destructor may block, so we set stream_ to NULL before calling the stream
        destructor. */
        rassert(stream_);
        repli_stream_t *stream_copy = stream_;
        stream_ = NULL;
        delete stream_copy;

        stream_exists_cond_.pulse();    // If anything was waiting for stream to close, signal it
        interrupt_streaming_cond_.pulse_if_non_null();   // Will interrupt any running backfill/stream operation

        // TODO: This might fail for future versions of the order source, which
        // require a backfill to have begun before it can be done.
        order_source->backfill_done();
    }

    void do_backfill_and_realtime_stream(repli_timestamp since_when);

#ifndef NDEBUG
    static bool inside_backfill_done_or_backfill;
#endif

private:

    void destroy_existing_slave_conn_if_it_exists();

    // The stream to the slave, or NULL if there is no slave connected.
    repli_stream_t *stream_;

    sequence_group_t *seq_group_;

    const int listener_port_;
    // Listens for incoming slave connections.
    boost::scoped_ptr<tcp_listener_t> listener_;

    // The key value store.
    btree_key_value_store_t *const kvs_;

    replication_config_t replication_config_;

    // Pointers to the gates we use to allow/disallow gets and sets
    gated_get_store_t *const get_gate_;
    gated_set_store_interface_t *const set_gate_;
    boost::scoped_ptr<gated_get_store_t::open_t> get_permission_;
    boost::scoped_ptr<gated_set_store_interface_t::open_t> set_permission_;

    // For reverse-backfilling;
    backfill_storer_t backfill_storer_;

    // This is unpulsed iff stream_ is non-NULL.
    resettable_cond_t stream_exists_cond_; 

    //
    mutex_t stream_setup_teardown_;

    // This is unpulsed iff there is not a running backfill/stream operation
    resettable_cond_t streaming_cond_;

    // Pulse this to interrupt a running backfill/realtime stream operation
    cond_weak_ptr_t interrupt_streaming_cond_;

    // TODO: Instead of having this, we should just remember if a slave was connected when we last
    // shut down.
    friend class dont_wait_for_slave_control_t;
    struct dont_wait_for_slave_control_t : public control_t {
        master_t *master;
        dont_wait_for_slave_control_t(master_t *m) :
            control_t("dont-wait-for-slave", "Go ahead and accept operations even though no slave "
                "has connected yet. Only use this if no slave was connected to the master at the "
                "time the master was last shut down. If you abuse this, the server could lose data "
                "or could serve out-of-date or inconsistent data to your clients.\r\n"),
            master(m) { }
        std::string call(int argc, UNUSED char **argv) {
            if (argc != 1) {
                return "\"dont-wait-for-slave\" doesn't expect any arguments.\r\n";
            }
            if (!master->get_permission_) {
                if (!master->stream_) {
                    master->get_permission_.reset(new gated_get_store_t::open_t(master->get_gate_));
                    master->set_permission_.reset(new gated_set_store_interface_t::open_t(master->set_gate_));
                    logINF("Now accepting operations even though no slave connected because "
                        "\"rethinkdb dont-wait-for-slave\" was run.\n");
                    return "Master will now accept operations even though no slave has connected yet.\r\n";
                } else {
                    return "The master cannot accept operations because it is reverse-backfilling from "
                        "the slave right now, so its data is in an inconsistent state. The master will "
                        "accept operations once it is done reverse-backfilling.\r\n";
                }
            } else {
                return "The master is already accepting operations.\r\n";
            }
        }
    } dont_wait_for_slave_control;

    DISABLE_COPYING(master_t);
};

}  // namespace replication

#endif  // __REPLICATION_MASTER_HPP__
