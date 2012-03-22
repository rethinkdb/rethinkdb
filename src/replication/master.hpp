#ifndef __REPLICATION_MASTER_HPP__
#define __REPLICATION_MASTER_HPP__

#include "concurrency/mutex.hpp"
#include "memcached/store.hpp"
#include "replication/backfill_in.hpp"
#include "replication/backfill_out.hpp"
#include "replication/backfill_receiver.hpp"
#include "replication/backfill_sender.hpp"
#include "replication/net_structs.hpp"
#include "replication/protocol.hpp"
#include "server/cmd_args.hpp"
#include "server/gated_store.hpp"

class btree_key_value_store_t;

namespace replication {

// master_t is a class that manages a connection to a slave.

class master_t :
    public backfill_sender_t,
    public backfill_receiver_t {
public:
    master_t(sequence_group_t *replication_seq_group, int port, btree_key_value_store_t *kv_store, replication_config_t replication_config, gated_get_store_t *get_gate, gated_set_store_interface_t *set_gate, backfill_receiver_order_source_t *master_order_source);

    ~master_t();

    bool has_slave() { return stream_ != NULL; }

    // Listener callback functions
    void on_conn(boost::scoped_ptr<nascent_tcp_conn_t>& conn);

    void hello(UNUSED net_hello_t message) { debugf("Received hello from slave.\n"); }

    void send(scoped_malloc<net_introduce_t>& message);
    void send(scoped_malloc<net_backfill_t>& message);
    using backfill_receiver_t::send;

    void send(scoped_malloc<net_timebarrier_t>& message) {
        timebarrier_helper(*message);
    }

    void conn_closed();

    void do_backfill_and_realtime_stream(repli_timestamp_t since_when);

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
    cond_t *interrupt_streaming_cond_;

    // TODO: Instead of having this, we should just remember if a slave was connected when we last
    // shut down.
    friend class dont_wait_for_slave_control_t;
    struct dont_wait_for_slave_control_t : public control_t {
        master_t *master;
        explicit dont_wait_for_slave_control_t(master_t *m) :
            control_t("dont-wait-for-slave", "Go ahead and accept operations even though no slave "
                "has connected yet. Only use this if no slave was connected to the master at the "
                "time the master was last shut down. If you abuse this, the server could lose data "
                "or could serve out-of-date or inconsistent data to your clients.\r\n"),
            master(m) { }
        std::string call(int argc, char **argv);
    };

    dont_wait_for_slave_control_t dont_wait_for_slave_control;

    DISABLE_COPYING(master_t);
};

}  // namespace replication

#endif  // __REPLICATION_MASTER_HPP__
