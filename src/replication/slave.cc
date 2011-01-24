#include "slave.hpp"
#include <stdint.h>
#include "net_structs.hpp"
#include "arch/linux/coroutines.hpp"

namespace replication {


// TODO unit test offsets

slave_t::slave_t(store_t *internal_store, replication_config_t config) 
    : internal_store(internal_store), 
      config(config), 
      conn(new tcp_conn_t(config.hostname, config.port)), 
      respond_to_queries(false), 
      timeout(INITIAL_TIMEOUT)
{
    failover.add_callback(this);
    coro_t::move_to_thread(get_num_threads() - 2);
    parser.parse_messages(conn, this);
}

slave_t::~slave_t() {
    struct : public message_parser_t::message_parser_shutdown_callback_t, public cond_t {
            void on_parser_shutdown() { pulse(); }
    } parser_shutdown_cb;

    if (!parser.shutdown(&parser_shutdown_cb))
        parser_shutdown_cb.wait();

    coro_t::move_to_thread(get_num_threads() - 2);
    delete conn;

    coro_t::move_to_thread(home_thread);
    
    /* cancel the timer */
    if(timer_token) cancel_timer(timer_token);
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
    timer_token = fire_timer_once(timeout, &reconnect_timer_callback, this);
}

void slave_t::reconnect_timer_callback(void *ctx) {
    slave_t *self = static_cast<slave_t*>(ctx);
    self->timer_token = NULL;
    try {
        if (self->conn) {
            delete self->conn; //put the old conn out of his misery, he's done here.
            self->conn = NULL; //make sure we don't double delete
        }

        /* reset everything */
        self->conn = new tcp_conn_t(self->config.hostname, self->config.port);
        self->timeout = INITIAL_TIMEOUT;

        /* we've succeeded, it's time to start parsing again */
        logINF("Successfully reconnected to the server\n");
        self->failover.on_resume();
        self->parser.parse_messages(self->conn, self);
    } catch (tcp_conn_t::connect_failed_exc_t& e) {
        logINF("Reconnection attempt failed\n");
        self->timeout *= TIMEOUT_GROWTH_FACTOR; //Increase the timeout
        self->timer_token = fire_timer_once(self->timeout, self->reconnect_timer_callback, self); //Set another timer to retry
    }
}

/* failover callback */
void slave_t::on_failure() {
    respond_to_queries = true;
}

void slave_t::on_resume() {
    respond_to_queries = false;
}

/* state for failover */
bool respond_to_queries;
}  // namespace replication
