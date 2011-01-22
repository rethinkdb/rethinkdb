#include "slave.hpp"
#include <stdint.h>
#include "net_structs.hpp"
#include "arch/linux/coroutines.hpp"

namespace replication {


// TODO unit test offsets

slave_t::slave_t(store_t *internal_store, replication_config_t config) 
    : internal_store(internal_store), config(config), conn(config.hostname, config.port), respond_to_queries(false), n_retries(RETRY_ATTEMPTS)
{
    failover.add_callback(this);
    coro_t::move_to_thread(get_num_threads() - 2);
    parser.parse_messages(&conn, this);
}

slave_t::~slave_t() {}

bool slave_t::shutdown(shutdown_callback_t *cb) {
    parser.stop_parsing(); //TODO put a callback on this
    _cb = cb;

    coro_t::move_to_thread(get_num_threads() - 2);//TODO make this work even if we're on the thread here ugh ugh ugh
    if (conn.is_read_open()) conn.shutdown_read();
    if (conn.is_write_open()) conn.shutdown_write();

    coro_t::move_to_thread(home_thread);
    if(internal_store->shutdown(_cb))
        _cb->on_store_shutdown();

    return false;
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

void slave_t::replicate(replicant_t *cb, repli_timestamp cutoff)
{
    return internal_store->replicate(cb, cutoff);
}

void slave_t::stop_replicating(replicant_t *cb)
{
    return internal_store->stop_replicating(cb);
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
    int reconnects_done = 0;
    bool success = false;

    while (reconnects_done++ < n_retries) {
        try {
            conn = tcp_conn_t(config.hostname, config.port);
            logINF("Successfully reconnected to the server");
            success = true;
            parser.parse_messages(&conn, this);
        }
        catch (tcp_conn_t::connect_failed_exc_t& e) {
            logINF("Connection attempt: %d failed\n", reconnects_done);
        }
    }

    if(!success) failover.on_failure();
}

/* failover callback */
void slave_t::on_failure() {
    respond_to_queries = true;
}

/* state for failover */
bool respond_to_queries;
}  // namespace replication
