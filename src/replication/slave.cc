#include "slave.hpp"
#include <stdint.h>
#include "net_structs.hpp"
#include "arch/linux/coroutines.hpp"

namespace replication {


// TODO unit test offsets

slave_t::slave_t(store_t *internal_store, replication_config_t replication_config, failover_config_t failover_config) 
    : internal_store(internal_store), 
      replication_config(replication_config), 
      failover_config(failover_config),
      conn(new tcp_conn_t(replication_config.hostname, replication_config.port)), 
      failover_script(failover_config.failover_script_path),
      respond_to_queries(false), 
      timeout(INITIAL_TIMEOUT),
      given_up(false),
      failover_reset_control(std::string("failover reset"), this),
      new_master_control(std::string("new master"), this)
{
    failover.add_callback(this);
    failover.add_callback(&failover_script);
    give_up.on_reconnect();

    coro_t::move_to_thread(get_num_threads() - 2);
    parser.parse_messages(conn, this);
}

slave_t::~slave_t() {
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

    {
        on_thread_t thread_switch(get_num_threads() - 2);
        delete conn;
        conn = NULL;
    }
}

store_t::get_result_t slave_t::get(store_key_t *key)
{
    return internal_store->get(key);
}

store_t::get_result_t slave_t::get_cas(store_key_t *key)
{
    return internal_store->get(key);
}

store_t::set_result_t slave_t::set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime) {
    if (respond_to_queries)
        return internal_store->set(key, data, flags, exptime);
    else
        return store_t::sr_not_allowed;
}

store_t::set_result_t slave_t::add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime)
{
    return internal_store->add(key, data,  flags,  exptime);
}

store_t::set_result_t slave_t::replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime)
{
    if (respond_to_queries)
        return internal_store->replace(key, data, flags, exptime);
    else
        return store_t::sr_not_allowed;
}

store_t::set_result_t slave_t::cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique)
{
    if (respond_to_queries)
        return internal_store->cas(key, data, flags, exptime, unique);
    else
        return store_t::sr_not_allowed;
}

store_t::incr_decr_result_t slave_t::incr(store_key_t *key, unsigned long long amount)
{
    if (respond_to_queries)
        return internal_store->incr(key, amount);
    else
        return store_t::incr_decr_result_t::idr_not_allowed;
}

store_t::incr_decr_result_t slave_t::decr(store_key_t *key, unsigned long long amount)
{
    if (respond_to_queries)
        return internal_store->decr(key, amount);
    else
        return store_t::incr_decr_result_t::idr_not_allowed;
}

store_t::append_prepend_result_t slave_t::append(store_key_t *key, data_provider_t *data)
{
    if (respond_to_queries)
        return internal_store->append(key, data);
    else
        return store_t::apr_not_allowed;
}

store_t::append_prepend_result_t slave_t::prepend(store_key_t *key, data_provider_t *data)
{
    if (respond_to_queries)
        return internal_store->prepend(key, data);
    else
        return store_t::apr_not_allowed;
}

store_t::delete_result_t slave_t::delete_key(store_key_t *key)
{
    if (respond_to_queries)
        return internal_store->delete_key(key);
    else
        return dr_not_allowed;
}

std::string slave_t::failover_reset() {
    give_up.reset();
    kill_conn();
    if (given_up)
    {
        /* the failover module needs a pulse */
        on_thread_t thread_switcher(get_num_threads() - 2);
        timer_token = fire_timer_once(timeout, &reconnect_timer_callback, this);
    }
    return std::string("Reseting failover\n");
}

 /* message_callback_t interface */
void slave_t::hello(boost::scoped_ptr<hello_message_t>& message)
{}
void slave_t::send(boost::scoped_ptr<backfill_message_t>& message)
{}
void slave_t::send(boost::scoped_ptr<announce_message_t>& message)
{}
void slave_t::send(boost::scoped_ptr<set_message_t>& message)
{}
void slave_t::send(boost::scoped_ptr<append_message_t>& message)
{}
void slave_t::send(boost::scoped_ptr<prepend_message_t>& message)
{}
void slave_t::send(boost::scoped_ptr<nop_message_t>& message)
{}
void slave_t::send(boost::scoped_ptr<ack_message_t>& message)
{}
void slave_t::send(boost::scoped_ptr<shutting_down_message_t>& message)
{}
void slave_t::send(boost::scoped_ptr<goodbye_message_t>& message)
{}
void slave_t::conn_closed()
{
    failover.on_failure();
    if (!give_up.give_up()) {
        timer_token = fire_timer_once(timeout, &reconnect_timer_callback, this);
    } else {
        logINF("The master has failed %d times in the last %d seconds, going rogue.\n", MAX_RECONNECTS_PER_N_SECONDS, N_SECONDS); //TODO when runtime commands and clas are in place to undo this put info on how to do it here
        given_up = true;
    }
}

/* failover driving functions */
void slave_t::reconnect_timer_callback(void *ctx) {
    slave_t *self = static_cast<slave_t*>(ctx);
    self->timer_token = NULL;
    try {
        if (self->conn) {
            delete self->conn; //put the old conn out of his misery, he's done here.
            self->conn = NULL; //make sure we don't double delete
        }

        /* reset everything */
        self->conn = new tcp_conn_t(self->replication_config.hostname, self->replication_config.port);
        self->timeout = INITIAL_TIMEOUT;

        /* we've succeeded, it's time to start parsing again */
        logINF("Successfully reconnected to the server\n");
        self->give_up.on_reconnect();
        self->failover.on_resume();

        /* restart the parser */
        self->parser.parse_messages(self->conn, self);
    } catch (tcp_conn_t::connect_failed_exc_t& e) {
        logINF("Reconnection attempt failed\n");
        self->timeout *= TIMEOUT_GROWTH_FACTOR; //Increase the timeout
        self->timer_token = fire_timer_once(self->timeout, self->reconnect_timer_callback, self); //Set another timer to retry
    }
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
