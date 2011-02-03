#ifndef __MEMCACHED_MEMCACHED_HPP__
#define __MEMCACHED_MEMCACHED_HPP__

#include "arch/arch.hpp"

struct store_t;

void serve_memcache(tcp_conn_t *conn, store_t *store);

#endif /* __MEMCACHED_MEMCACHED_HPP__ */
