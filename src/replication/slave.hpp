#ifndef __REPLICATION_SLAVE_HPP__
#define __REPLICATION_SLAVE_HPP__

#include "replication/protocol.hpp"
#include "server/cmd_args.hpp"
#include "store.hpp"
#include "failover.hpp"
#include <queue>
#include "control.hpp"

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

/* The slave_t class is responsible for connecting to another RethinkDB process
 * and pushing the values that it receives in to its internal store. It also
 * handles the failover module if failover is enabled. Currently the slave_t
 * also is itself a failover callback and derives from the store_t class, thus
 * allowing it to modify the behaviour of the store. */

/* It has been suggested that the failover callback and replication
 * functionality of the slave_t class be separated. I don't think this is such
 * a bad idea but it doesn't seem particularly urgent to me right now. */

class slave_t :
    public home_thread_mixin_t,
    public set_store_interface_t,
    public failover_callback_t,
    public message_callback_t
{
public:
    friend void run(slave_t *);

    slave_t(btree_key_value_store_t *, replication_config_t, failover_config_t);
    ~slave_t();

    /* set_store_interface_t interface. This interface will not work properly for anything until
    we fail over. */

    mutation_result_t change(const mutation_t &m);

    /* message_callback_t interface */
    // These call .swap on their parameter, taking ownership of the pointee.
    void hello(net_hello_t message);
    void send(buffed_data_t<net_backfill_t>& message);
    void send(buffed_data_t<net_announce_t>& message);
    void send(buffed_data_t<net_get_cas_t>& message);
    void send(stream_pair<net_sarc_t>& message);
    void send(stream_pair<net_backfill_set_t>& message);
    void send(buffed_data_t<net_incr_t>& message);
    void send(buffed_data_t<net_decr_t>& message);
    void send(stream_pair<net_append_t>& message);
    void send(stream_pair<net_prepend_t>& message);
    void send(buffed_data_t<net_delete_t>& message);
    void send(buffed_data_t<net_nop_t>& message);
    void send(buffed_data_t<net_ack_t>& message);
    void send(buffed_data_t<net_shutting_down_t>& message);
    void send(buffed_data_t<net_goodbye_t>& message);
    void conn_closed();

    /* failover module which is alerted by an on_failure() call when we go out
     * of contact with the master */
    failover_t failover;

private:
    friend class failover_t;

    /* failover callback interface */
    void on_failure();
    void on_resume();


    static void reconnect_timer_callback(void *ctx);

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

    void kill_conn();


    // This is too complicated.

    give_up_t give_up_;

    /* Other failover callbacks */
    failover_script_callback_t failover_script_;

    /* state for failover */
    bool respond_to_queries_; /* are we responding to queries */
    long timeout_; /* ms to wait before trying to reconnect */

    timer_token_t *reconnection_timer_token_;

    failover_reset_control_t failover_reset_control_;


    new_master_control_t new_master_control_;

    coro_t *coro_;

    btree_key_value_store_t *internal_store_;
    replication_config_t replication_config_;
    failover_config_t failover_config_;

    repli_stream_t *stream_;
    bool shutting_down_;
};

void run(slave_t *); //TODO make this static and private

}  // namespace replication

#endif  // __REPLICATION_SLAVE_HPP__
