#ifndef __REPLICATION_SLAVE_HPP__
#define __REPLICATION_SLAVE_HPP__

#include "replication/protocol.hpp"
#include "server/cmd_args.hpp"
#include "store.hpp"
#include "failover.hpp"
#include <queue>
#include "control.hpp"

#define INITIAL_TIMEOUT  (100) //initial time we wait reconnect to the master server on failure in ms
#define TIMEOUT_GROWTH_FACTOR   (2) //every failed reconnect the timeout increase by this factor
#define TIMEOUT_CAP (1000*60*2) //until it reaches the cap (that's 2 minutes over there)

/* if we mave more than MAX_RECONNECTS_PER_N_SECONDS in N_SECONDS then we give
 * up on the master server for a longer time (possibly until the user tells us
 * to stop) */
#define N_SECONDS (5*60)
#define MAX_RECONNECTS_PER_N_SECONDS (5)


namespace replication {

/* The slave_t class is responsible for connecting to another RethinkDB process
 * and pushing the values that it receives in to its internal store. It also
 * handles the failover module if failover is enabled. Currently the slave_t
 * also is itself a failover callback and derives from the store_t class, thus
 * allowing it to modify the behaviour of the store. */ 

/* It has been suggested that the failover callback and replication
 * functionality of the slave_t class be separated. I don't think this is such
 * a bad idea but it doesn't seem particularly urgent to me right now. */

struct slave_t :
    public home_thread_mixin_t,
    public store_t,
    public failover_callback_t,
    public message_callback_t
{
friend void run(slave_t *);
public:
    slave_t(store_t *, replication_config_t, failover_config_t);
    ~slave_t();

    /* failover module which is alerted by an on_failure() call when we go out
     * of contact with the master */
    failover_t failover;

private:
    void kill_conn();
    coro_t *coro;

private:
    store_t *internal_store;
    replication_config_t replication_config;
    failover_config_t failover_config;
    tcp_conn_t *conn;
    message_parser_t parser;
    bool shutting_down;

public:
    /* store_t interface. */

    get_result_t get(store_key_t *key);
    get_result_t get_cas(store_key_t *key, castime_t castime);
    rget_result_t rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open, uint64_t max_results);
    set_result_t set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, castime_t castime);
    incr_decr_result_t incr(store_key_t *key, unsigned long long amount, castime_t castime);
    incr_decr_result_t decr(store_key_t *key, unsigned long long amount, castime_t castime);
    append_prepend_result_t append(store_key_t *key, data_provider_t *data, castime_t castime);
    append_prepend_result_t prepend(store_key_t *key, data_provider_t *data, castime_t castime);
    delete_result_t delete_key(store_key_t *key, repli_timestamp timestamp);

public:
    /* message_callback_t interface */
    // These call .swap on their parameter, taking ownership of the pointee.
    void hello(net_hello_t message);
    void send(buffed_data_t<net_backfill_t>& message);
    void send(buffed_data_t<net_announce_t>& message);
    void send(stream_pair<net_set_t>& message);
    void send(stream_pair<net_append_t>& message);
    void send(stream_pair<net_prepend_t>& message);
    void send(buffed_data_t<net_nop_t>& message);
    void send(buffed_data_t<net_ack_t>& message);
    void send(buffed_data_t<net_shutting_down_t>& message);
    void send(buffed_data_t<net_goodbye_t>& message);
    void conn_closed();

private:
    friend class failover_t;

    /* failover callback interface */
    void on_failure();
    void on_resume();


private:
    /* Other failover callbacks */
    failover_script_callback_t failover_script;

private:
    /* state for failover */
    bool respond_to_queries; /* are we responding to queries */
    long timeout; /* ms to wait before trying to reconnect */

    static void reconnect_timer_callback(void *ctx);
    timer_token_t *timer_token;

    /* structure to tell us when to give up on the master */
    struct give_up_t {
    private:
        std::queue<float> succesful_reconnects;
    public:
        void on_reconnect();
        bool give_up();
        void reset();
    private:
        void limit_to(unsigned int limit);
    } give_up;

    /* Failover controllers */

private:
    /* Control to  allow the failover state to be reset during run time */
    std::string failover_reset();

    struct failover_reset_control_t
        : public control_t
    {
    private:
        slave_t *slave;
    public:
        failover_reset_control_t(std::string key, slave_t *slave)
            : control_t(key, "Reset the failover module to the state at startup (will force a reconnection to the master)."), slave(slave)
        {}
        std::string call(std::string);
    };
    failover_reset_control_t failover_reset_control;

private:
    /* Control to allow the master to be changed during run time */
    std::string new_master(std::string args);

    struct new_master_control_t
        : public control_t
    {
    private:
        slave_t *slave;
    public:
        new_master_control_t(std::string key, slave_t *slave)
            : control_t(key, "Set a new master for replication (the slave will disconnect and immediately reconnect to the new server). Syntax: \"rdb new master: host port\""), slave(slave)
    {}
        std::string call(std::string);
    };
    new_master_control_t new_master_control;
};

void run(slave_t *); //TODO make this static and private

}  // namespace replication

#endif  // __REPLICATION_SLAVE_HPP__
