#ifndef __PROTOCOL_REDIS_PROTO_HPP__
#define __PROTOCOL_REDIS_PROTO_HPP__

#include <iostream>
#include "arch/arch.hpp"
#include "store.hpp"

//void serve_redis(std::iostream &redis_stream, get_store_t *get_store, set_store_interface_t *set_store);
void serve_redis(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store);

#endif /* __PROTOCOL_REDIS_PROTO_HPP__*/
