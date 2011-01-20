#ifndef __REPLICATION_SLAVE_HPP__
#define __REPLICATION_SLAVE_HPP__

#include <boost/scoped_ptr.hpp>
#undef assert

#include "arch/arch.hpp"
#include "replication/messages.hpp"
#include "config/cmd_args.hpp"
#include "store.hpp"
#include "failover.hpp"

#define RETRY_ATTEMPTS 10 //TODO move me

namespace replication {

class message_callback_t {
public:
    // These call .swap on their parameter, taking ownership of the pointee.
    virtual void hello(boost::scoped_ptr<hello_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<backfill_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<announce_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<set_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<append_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<prepend_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<nop_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<ack_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<shutting_down_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<goodbye_message_t>& message) = 0;
    virtual void conn_closed() = 0;
};

struct btree_replica_t :
    public home_thread_mixin_t,
    public store_t,
    public failover_callback_t,
    public message_callback_t
{
public:
    btree_replica_t(store_t *, standard_serializer_t::shutdown_callback_t *, replication_config_t *);
    ~btree_replica_t();

private:
    store_t *internal_store;
    replication_config_t *config;
    tcp_conn_t conn;

    failover_t failover;

public:
    /* store_t interface. */

    void get(store_key_t *key, get_callback_t *cb);
    void get_cas(store_key_t *key, get_callback_t *cb);
    void set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb);
    void add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb);
    void replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb);
    void cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, set_callback_t *cb);
    void incr(store_key_t *key, unsigned long long amount, incr_decr_callback_t *cb);
    void decr(store_key_t *key, unsigned long long amount, incr_decr_callback_t *cb);
    void append(store_key_t *key, data_provider_t *data, append_prepend_callback_t *cb);
    void prepend(store_key_t *key, data_provider_t *data, append_prepend_callback_t *cb);
    void delete_key(store_key_t *key, delete_callback_t *cb);
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
};

void parse_messages(tcp_conn_t *conn, message_callback_t *receiver);

}  // namespace replication




#endif  // __REPLICATION_SLAVE_HPP__
