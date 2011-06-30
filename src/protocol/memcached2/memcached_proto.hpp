#ifndef __MEMCACHED_MEMCACHED_PROTO_HPP__
#define __MEMCACHED_MEMCACHED_PROTO_HPP__

#include "arch/arch.hpp"
#include "store.hpp"

void serve_memcache2(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store);

#endif /* __MEMCACHED_MEMCACHED_PROTO_HPP__ */
