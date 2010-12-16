
#ifndef __REPLICATION_MASTER_HPP__
#define __REPLICATION_MASTER_HPP__

#include "store.hpp"
#include "concurrency/mutex.hpp"
#include "conn_acceptor.hpp"

struct replication_master_t;

struct replication_message_t :
    public home_thread_mixin_t,
    public thread_message_t,
    public lock_available_callback_t,
    public net_conn_write_external_callback_t
{
    replication_message_t(
        replication_master_t *parent,
        const store_key_t *k,
        const_buffer_group_t *bg,
        store_t::replicant_t::done_callback_t *cb,
        mcflags_t f,
        exptime_t e,
        cas_t c);

    replication_master_t *parent;

    const store_key_t *key;
    const_buffer_group_t *buffer_group;
    store_t::replicant_t::done_callback_t *callback;
    mcflags_t flags;
    exptime_t exptime;
    cas_t cas;

    bool sent;
    char header[400];
    int which_buffer_of_group;

    void on_lock_available();
    void on_thread_switch();
    void on_net_conn_write_external();
    void on_net_conn_close();
    void done_sending();
};

struct replication_master_t :
    public store_t::replicant_t,
    public conn_handler_t,
    public home_thread_mixin_t
{
    replication_master_t(store_t *s, net_conn_t *c);

    store_t *store;
    net_conn_t *conn;
    mutex_t conn_write_mutex;

    // Callbacks as a replicant_t
    void value(
        const store_key_t *key,
        const_buffer_group_t *value, store_t::replicant_t::done_callback_t *cb,
        mcflags_t flags, exptime_t exptime, cas_t cas, repli_timestamp ts);
    void stopped();

    bool quitting;
    void quit();
};

/* If you create a conn_acceptor_t with this function and with a store_t* as the user data, then
it will serve replication to slaves. */
conn_handler_t *create_replication_master(net_conn_t *, void *);

#endif /* __REPLICATION_MASTER_HPP__ */

