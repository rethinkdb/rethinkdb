#ifndef __MEMCACHED_TCP_CONN_HPP__
#define __MEMCACHED_TCP_CONN_HPP__

#include "arch/types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "memcached/store.hpp"
#include "arch/io/network.hpp"

/* Serves memcache queries over the given TCP connection until the connection in question
is closed. */

void serve_memcache(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store);

/* Listens for TCP connections on the given port and serves memcache queries over those
connections until the destructor is called. */

struct memcache_listener_t : public home_thread_mixin_t {

    memcache_listener_t(int port, get_store_t *get_store, set_store_interface_t *set_store);

private:
    get_store_t *get_store;
    set_store_interface_t *set_store;
    int next_thread;

    /* We use this to make sure that all TCP connections stop when the
    `memcached_listener_t` is destroyed. */
    auto_drainer_t drainer;

    tcp_listener_t tcp_listener;

    void handle(auto_drainer_t::lock_t keepalive, boost::scoped_ptr<tcp_conn_t>& conn);
};

#endif /* __MEMCACHED_TCP_CONN_HPP__ */
