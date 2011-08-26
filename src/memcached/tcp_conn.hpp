#ifndef __MEMCACHED_TCP_CONN_HPP__
#define __MEMCACHED_TCP_CONN_HPP__

#include "arch/types.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/drain_semaphore.hpp"
#include "memcached/store.hpp"

/* Serves memcache queries over the given TCP connection until the connection in question
is closed. */

void serve_memcache(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store);

/* Listens for TCP connections on the given port and serves memcache queries over those
connections until the destructor is called. */

struct memcache_listener_t : public home_thread_mixin_t {

    memcache_listener_t(int port, get_store_t *get_store, set_store_interface_t *set_store);
    ~memcache_listener_t();

private:
    get_store_t *get_store;
    set_store_interface_t *set_store;
    cond_t pulse_to_begin_shutdown;
    drain_semaphore_t active_connection_drain_semaphore;
    int next_thread;
    boost::scoped_ptr<tcp_listener_t> tcp_listener;
    void handle(boost::scoped_ptr<tcp_conn_t>& conn);
};

#endif /* __MEMCACHED_TCP_CONN_HPP__ */
