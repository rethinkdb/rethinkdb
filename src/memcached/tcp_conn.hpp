#ifndef __MEMCACHED_TCP_CONN_HPP__
#define __MEMCACHED_TCP_CONN_HPP__

#include <string>
#include "arch/arch.hpp"
#include "store.hpp"
#include "concurrency/fifo_checker.hpp"

/* Serves memcache queries over the given TCP connection until the connection in question
is closed or until SIGINT is sent to the server. */

void serve_memcache(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store);

#endif /* __MEMCACHED_TCP_CONN_HPP__ */
