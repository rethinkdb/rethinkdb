#ifndef __REPLICATION_SLAVE_HPP__
#define __REPLICATION_SLAVE_HPP__

#include <queue>

#include "replication/protocol.hpp"
#include "replication/queueing_store.hpp"
#include "server/cmd_args.hpp"
#include "server/control.hpp"
#include "store.hpp"
#include "failover.hpp"

// The initial time we wait to reconnect to the master, upon failure.  In ms.
#define INITIAL_TIMEOUT  (100)

// Every failed reconnect, the timeout increases by this factor.
#define TIMEOUT_GROWTH_FACTOR 2

// But it can't surpass the cap.
#define TIMEOUT_CAP (1000*60*2)

/* if we mave more than MAX_RECONNECTS_PER_N_SECONDS in N_SECONDS then we give
 * up on the master server for a longer time (possibly until the user tells us
 * to stop) */
#define N_SECONDS (5*60)
#define MAX_RECONNECTS_PER_N_SECONDS (5)

/* This is a hack and we shouldn't be tied to this particular type */
struct btree_key_value_store_t;

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

    // Called by run() after the slave_stream_manager_t is created. Fold into constructor?
    void reverse_side_backfill(repli_timestamp since_when);

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

    // When conn_closed() is called, we consult interrupted_by_external_event_ to determine
    // if this is a spontaneous loss of connectivity or if it's due to an intentional
    // shutdown.
    bool interrupted_by_external_event_;

    // In our destructor, we block on shutdown_cond_ to make sure that the repli_stream_t
    // has actually completed its shutdown process.
    cond_t shutdown_cond_;
};

class slave_t :
    public home_thread_mixin_t,
    public set_store_interface_t,
    public failover_callback_t
{
public:
    friend void run(slave_t *);

    slave_t(btree_key_value_store_t *, replication_config_t, failover_config_t);
    ~slave_t();

    /* set_store_interface_t interface. This interface will not work properly for anything until
    we fail over. */

    mutation_result_t change(const mutation_t &m);

    /* failover module which is alerted by an on_failure() call when we go out
     * of contact with the master */
    failover_t failover;

private:
    friend class failover_t;

    /* failover callback interface */
    void on_failure();
    void on_resume();


    /* structure to tell us when to give up on the master */
    class give_up_t {
    public:
        void on_reconnect();
        bool give_up();
        void reset();
    private:
        void limit_to(unsigned int limit);
        std::queue<float> successful_reconnects;
    };

    /* Failover controllers */

    /* Control to  allow the failover state to be reset during run time */
    std::string failover_reset();

    class failover_reset_control_t : public control_t {
    public:
        failover_reset_control_t(std::string key, slave_t *slave)
            : control_t(key, "Reset the failover module to the state at startup (will force a reconnection to the master)."), slave(slave)
        {}
        std::string call(int argc, char **argv);
    private:
        slave_t *slave;
    };

    /* Control to allow the master to be changed during run time */
    std::string new_master(int argc, char **argv);

    class new_master_control_t : public control_t {
    public:
        new_master_control_t(std::string key, slave_t *slave)
            : control_t(key, "Set a new master for replication (the slave will disconnect and immediately reconnect to the new server). Syntax: \"rdb new_master host port\""), slave(slave)
    {}
        std::string call(int argc, char **argv);
    private:
        slave_t *slave;
    };

    // This is too complicated.

    give_up_t give_up_;

    /* Other failover callbacks */
    failover_script_callback_t failover_script_;

    /* state for failover */
    bool respond_to_queries_; /* are we responding to queries */
    long timeout_; /* ms to wait before trying to reconnect */

    failover_reset_control_t failover_reset_control_;

    new_master_control_t new_master_control_;

    boost::scoped_ptr<queueing_store_t> internal_store_;
    replication_config_t replication_config_;
    failover_config_t failover_config_;

    /* shutting_down_ points to a local variable within the run() coroutine; the
    destructor sets *shutting_down_ to true and pulse pulse_to_interrupt_run_loop_
    to shut down the slave. */
    bool *shutting_down_;

    /* pulse_to_interrupt_run_loop_ holds a pointer to whatever multicond_t the run
    loop is blocking on at the moment. */
    multicond_weak_ptr_t pulse_to_interrupt_run_loop_;
};

void run(slave_t *); //TODO make this static and private

}  // namespace replication

#endif  // __REPLICATION_SLAVE_HPP__
