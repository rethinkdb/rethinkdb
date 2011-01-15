#ifndef __MEMCACHED_MEMCACHED_HPP__
#define __MEMCACHED_MEMCACHED_HPP__

#include "arch/arch.hpp"

struct server_t;

void serve_memcache(net_conn_t *conn, server_t *server);

#endif /* __MEMCACHED_MEMCACHED_HPP__ */
