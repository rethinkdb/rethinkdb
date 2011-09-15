#ifndef __PROTOCOL_REDIS_PROTO_HPP__
#define __PROTOCOL_REDIS_PROTO_HPP__

#include <iostream>
#include "arch/arch.hpp"
#include "memcached/store.hpp"

#include "protocol/redis/redis.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/dummy_namespace_interface.hpp"

//void serve_redis(std::iostream &redis_stream, get_store_t *get_store, set_store_interface_t *set_store);
void serve_redis(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store, namespace_interface_t<redis_protocol_t> *);

struct redis_output_writer : home_thread_mixin_t {
    redis_output_writer(tcp_conn_t *conn) : out_conn(conn) {;}
    void output_response(redis_protocol_t::redis_return_type response);
protected:
    tcp_conn_t *out_conn;
};

#endif /* __PROTOCOL_REDIS_PROTO_HPP__*/
