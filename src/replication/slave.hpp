#ifndef __REPLICATION_SLAVE_HPP__
#define __REPLICATION_SLAVE_HPP__

#include "replication/protocol.hpp"
#include "server/cmd_args.hpp"
#include "store.hpp"
#include "failover.hpp"

#define RETRY_ATTEMPTS 10 //TODO move me

namespace replication {


struct slave_t :
    public home_thread_mixin_t,
    public store_t,
    public failover_callback_t,
    public message_callback_t,
    public thread_message_t
{
public:
    slave_t(store_t *, replication_config_t);
    ~slave_t();

private:
    store_t *internal_store;
    replication_config_t config;
    tcp_conn_t conn;

    failover_t failover;

public:
    /* store_t interface. */

    get_result_t get(store_key_t *key);
    get_result_t get_cas(store_key_t *key);
    set_result_t set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime);
    set_result_t add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime);
    set_result_t replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime);
    set_result_t cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique);
    incr_decr_result_t incr(store_key_t *key, unsigned long long amount);
    incr_decr_result_t decr(store_key_t *key, unsigned long long amount);
    append_prepend_result_t append(store_key_t *key, data_provider_t *data);
    append_prepend_result_t prepend(store_key_t *key, data_provider_t *data);
    delete_result_t delete_key(store_key_t *key);
    void replicate(replicant_t *cb, repli_timestamp cutoff);
    void stop_replicating(replicant_t *cb);

public:
    /* message_callback_t interface */
    void hello(boost::scoped_ptr<hello_message_t>& message);
    void send(boost::scoped_ptr<backfill_message_t>& message);
    void send(boost::scoped_ptr<announce_message_t>& message);
    void send(boost::scoped_ptr<set_message_t>& message);
    void send(boost::scoped_ptr<append_message_t>& message);
    void send(boost::scoped_ptr<prepend_message_t>& message);
    void send(boost::scoped_ptr<nop_message_t>& message);
    void send(boost::scoped_ptr<ack_message_t>& message);
    void send(boost::scoped_ptr<shutting_down_message_t>& message);
    void send(boost::scoped_ptr<goodbye_message_t>& message);
    void conn_closed();
    
public:
    /* failover callback */
    void on_failure();

private:
    /* state for failover */
    bool respond_to_queries;
    int n_retries;

public:
    void on_thread_switch() {
        parse_messages(&conn, this);
    }
};

}  // namespace replication




#endif  // __REPLICATION_SLAVE_HPP__
