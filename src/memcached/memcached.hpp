#ifndef __MEMCACHED_MEMCACHED_HPP__
#define __MEMCACHED_MEMCACHED_HPP__

#include "arch/arch.hpp"

class cas_generator_t;
struct store_t;

void serve_memcache(tcp_conn_t *conn, store_t *store, cas_generator_t *cas_gen);

#endif /* __MEMCACHED_MEMCACHED_HPP__ */
