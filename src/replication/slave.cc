#include "slave.hpp"
#include <stdint.h>
#include "net_structs.hpp"

namespace replication {


// TODO unit test offsets

slave_t::slave_t(store_t *internal_store, replication_config_t config) 
    : internal_store(internal_store), config(config), conn(config.hostname, config.port), respond_to_queries(false), n_retries(RETRY_ATTEMPTS)
{
    failover.add_callback(this);
    continue_on_thread(home_thread, this);
}

slave_t::~slave_t() {}

/* store interface */
/*
void slave_t::get(store_key_t *key, get_callback_t *cb)
{
    internal_store->get(key,cb);
}

void slave_t::get_cas(store_key_t *key, get_callback_t *cb)
{
    internal_store->get_cas(key,cb);
}

void slave_t::set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb)
{
    if (respond_to_queries)
        internal_store->set(key, data, flags, exptime, cb);
    else
        cb->not_allowed();
}

void slave_t::add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb)
{
    if (respond_to_queries)
        internal_store->add(key, data, flags, exptime, cb);
    else
        cb->not_allowed();
}

void slave_t::replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb)
{
    if (respond_to_queries)
        internal_store->replace(key, data, flags, exptime, cb);
    else
        cb->not_allowed();
}

void slave_t::cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, set_callback_t *cb)
{
    if (respond_to_queries)
        internal_store->cas(key, data, flags, exptime, unique, cb);
    else
        cb->not_allowed();
}

void slave_t::incr(store_key_t *key, unsigned long long amount, incr_decr_callback_t *cb)
{
    if (respond_to_queries)
        internal_store->incr(key, amount, cb);
    else
        cb->not_allowed();
}

void slave_t::decr(store_key_t *key, unsigned long long amount, incr_decr_callback_t *cb)
{
    if (respond_to_queries)
        internal_store->decr(key, amount, cb);
    else
        cb->not_allowed();
}

void slave_t::append(store_key_t *key, data_provider_t *data, append_prepend_callback_t *cb)
{
    if (respond_to_queries)
        internal_store->append(key, data, cb);
    else
        cb->not_allowed();
}

void slave_t::prepend(store_key_t *key, data_provider_t *data, append_prepend_callback_t *cb)
{
    if (respond_to_queries)
        internal_store->prepend(key, data, cb);
    else
        cb->not_allowed();
}

void slave_t::delete_key(store_key_t *key, delete_callback_t *cb)
{
    if (respond_to_queries)
        internal_store->delete_key(key, cb);
    else
        cb->not_allowed();
}

void slave_t::replicate(replicant_t *cb, repli_timestamp cutoff)
{
    internal_store->replicate(cb, cutoff);
}

void slave_t::stop_replicating(replicant_t *cb)
{
    internal_store->stop_replicating(cb);
} */

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
            parse_messages(&conn, this);
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
