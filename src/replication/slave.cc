#include "slave.hpp"

#include <stdint.h>

#include "arch/linux/coroutines.hpp"
#include "logger.hpp"
#include "net_structs.hpp"

namespace replication {


// TODO unit test offsets

slave_t::slave_t(store_t *internal_store, replication_config_t replication_config, failover_config_t failover_config) 
    : internal_store(internal_store), 
      replication_config(replication_config), 
      failover_config(failover_config),
      conn(NULL),
      shutting_down(false),
      failover_script(failover_config.failover_script_path),
      timeout(INITIAL_TIMEOUT),
      timer_token(NULL),
      failover_reset_control(std::string("failover reset"), this),
      new_master_control(std::string("new master"), this)
{
    coro_t::spawn(boost::bind(&run, this));
}

slave_t::~slave_t() {
    shutting_down = true;
    coro_t::move_to_thread(home_thread);
    kill_conn();
    
    /* cancel the timer */
    if(timer_token) cancel_timer(timer_token);
}

void slave_t::kill_conn() {
    struct : public message_parser_t::message_parser_shutdown_callback_t, public cond_t {
            void on_parser_shutdown() { pulse(); }
    } parser_shutdown_cb;

    {
        on_thread_t thread_switch(home_thread);

        if (!parser.shutdown(&parser_shutdown_cb))
            parser_shutdown_cb.wait();
    }

    { //on_thread_t scope
        on_thread_t thread_switch(get_num_threads() - 2);
        if (conn) { 
            delete conn;
            conn = NULL;
        }
    }
}

get_result_t slave_t::get(store_key_t *key)
{
    return internal_store->get(key);
}

get_result_t slave_t::get_cas(store_key_t *key, castime_t castime)
{
    return internal_store->get_cas(key, castime);
}

rget_result_ptr_t slave_t::rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open) {
    return internal_store->rget(start, end, left_open, right_open);
}

set_result_t slave_t::sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    if (respond_to_queries) {
        return internal_store->sarc(key, data, flags, exptime, castime, add_policy, replace_policy, old_cas);
    } else {
        return sr_not_allowed;
    }
}

incr_decr_result_t slave_t::incr_decr(incr_decr_kind_t kind, store_key_t *key, uint64_t amount, castime_t castime)
{
    if (respond_to_queries)
        return internal_store->incr_decr(kind, key, amount, castime);
    else
        return incr_decr_result_t::idr_not_allowed;
}

append_prepend_result_t slave_t::append_prepend(append_prepend_kind_t kind, store_key_t *key, data_provider_t *data, castime_t castime)
{
    if (respond_to_queries)
        return internal_store->append_prepend(kind, key, data, castime);
    else
        return apr_not_allowed;
}

delete_result_t slave_t::delete_key(store_key_t *key, repli_timestamp timestamp)
{
    if (respond_to_queries)
        return internal_store->delete_key(key, timestamp);
    else
        return dr_not_allowed;
}

std::string slave_t::failover_reset() {
    on_thread_t thread_switch(get_num_threads() - 2);
    give_up.reset();
    timeout = INITIAL_TIMEOUT;

    if (timer_token) {
        cancel_timer(timer_token);
        timer_token = NULL;
    }

    if (conn) kill_conn(); //this will cause a notify
    else coro->notify();

    return std::string("Reseting failover\n");
}

 /* message_callback_t interface */
void slave_t::hello(net_hello_t message) { }
void slave_t::send(buffed_data_t<net_backfill_t>& message) { }
void slave_t::send(buffed_data_t<net_announce_t>& message) { }
void slave_t::send(buffed_data_t<net_get_cas_t>& message) { }
void slave_t::send(stream_pair<net_set_t>& message) { }
void slave_t::send(stream_pair<net_add_t>& message) { }
void slave_t::send(stream_pair<net_replace_t>& message) { }
void slave_t::send(stream_pair<net_cas_t>& message) { }
void slave_t::send(buffed_data_t<net_incr_t>& message) { }
void slave_t::send(buffed_data_t<net_decr_t>& message) { }
void slave_t::send(stream_pair<net_append_t>& message) { }
void slave_t::send(stream_pair<net_prepend_t>& message) { }
void slave_t::send(buffed_data_t<net_delete_t>& message) { }
void slave_t::send(buffed_data_t<net_nop_t>& message) { }
void slave_t::send(buffed_data_t<net_ack_t>& message) { }
void slave_t::send(buffed_data_t<net_shutting_down_t>& message) { }
void slave_t::send(buffed_data_t<net_goodbye_t>& message) { }
void slave_t::conn_closed() {
    coro->notify();
}

/* failover driving functions */
void slave_t::reconnect_timer_callback(void *ctx) {
    slave_t *self = static_cast<slave_t*>(ctx);
    self->coro->notify();
}

/* give_up_t interface: the give_up_t struct keeps track of the last N times
 * the server failed. If it's failing to frequently then it will tell us to
 * give up on the server and stop trying to reconnect */
void slave_t::give_up_t::on_reconnect() {
    succesful_reconnects.push(ticks_to_secs(get_ticks()));
    limit_to(MAX_RECONNECTS_PER_N_SECONDS);
}

bool slave_t::give_up_t::give_up() {
    limit_to(MAX_RECONNECTS_PER_N_SECONDS);

    return (succesful_reconnects.size() == MAX_RECONNECTS_PER_N_SECONDS && (succesful_reconnects.back() - ticks_to_secs(get_ticks())) < N_SECONDS);
}

void slave_t::give_up_t::reset() {
    limit_to(0);
}

void slave_t::give_up_t::limit_to(unsigned int limit) {
    while (succesful_reconnects.size() > limit)
        succesful_reconnects.pop();
}

/* failover callback */
void slave_t::on_failure() {
    respond_to_queries = true;
}

void slave_t::on_resume() {
    respond_to_queries = false;
}

void run(slave_t *slave) {
    slave->coro = coro_t::self();
    slave->failover.add_callback(slave);
    slave->respond_to_queries = false;
    bool first_connect = true;
    while (!slave->shutting_down) {
        try {
            if (slave->conn) { 
                delete slave->conn;
                slave->conn = NULL;
            }
            coro_t::move_to_thread(get_num_threads() - 2);
            logINF("Attempting to connect as slave to: %s:%d\n", slave->replication_config.hostname, slave->replication_config.port);
            slave->conn = new tcp_conn_t(slave->replication_config.hostname, slave->replication_config.port);
            slave->timeout = INITIAL_TIMEOUT;
            slave->give_up.on_reconnect();

            slave->parser.parse_messages(slave->conn, slave);
            if (!first_connect) //UGLY :( but it's either this or code duplication
                slave->failover.on_resume(); //Call on_resume if we've failed before
            first_connect = false;
            logINF("Connected as slave to: %s:%d\n", slave->replication_config.hostname, slave->replication_config.port);


            coro_t::wait(); //wait for things to fail
            slave->failover.on_failure();
            if (slave->shutting_down) break;

        } catch (tcp_conn_t::connect_failed_exc_t& e) {
            //Presumably if the master doesn't even accept an initial
            //connection then this is a user error rather than some sort of
            //failure
            if (first_connect)
                crash("Master at %s:%d is not responding :(. Perhaps you haven't brought it up yet. But what do I know I'm just a database\n", slave->replication_config.hostname, slave->replication_config.port); 
        }

        /* The connection has failed. Let's see what se should do */
        if (!slave->give_up.give_up()) {
            slave->timer_token = fire_timer_once(slave->timeout, &slave->reconnect_timer_callback, slave);

            slave->timeout *= TIMEOUT_GROWTH_FACTOR;
            if (slave->timeout > TIMEOUT_CAP) slave->timeout = TIMEOUT_CAP;

            coro_t::wait(); //waiting on a timer
            slave->timer_token = NULL;
        } else {
            logINF("Master at %s:%d has failed %d times in the last %d seconds," \
                    "going rogue. To resume slave behavior send the command" \
                    "\"rethinkdb failover reset\" (over telnet)\n",
                    slave->replication_config.hostname, slave->replication_config.port,
                    MAX_RECONNECTS_PER_N_SECONDS, N_SECONDS); 
            coro_t::wait(); //the only thing that can save us now is a reset
        }
    }
}

std::string slave_t::new_master(std::string args) {
    std::string host = args.substr(0, args.find(' '));
    if (host.length() >  MAX_HOSTNAME_LEN - 1)
        return std::string("That hostname is soo long, use a shorter one\n");

    /* redo the replication_config info */
    strcpy(replication_config.hostname, host.c_str());
    replication_config.port = atoi(strip_spaces(args.substr(args.find(' ') + 1)).c_str()); //TODO this is ugly

    failover_reset();
    
    return std::string("New master set\n");
}

std::string slave_t::failover_reset_control_t::call(std::string) {
    return slave->failover_reset();
}

std::string slave_t::new_master_control_t::call(std::string args) {
    return slave->new_master(args);
}

}  // namespace replication
