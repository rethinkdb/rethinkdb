#ifndef __PROTOCOL_HPP__
#define __PROTOCOL_HPP__

#include "arch/arch.hpp"
#include "memcached/store.hpp"
#include <iostream>

struct protocol_listener_t;

typedef void (*tcp_serve_func)(tcp_conn_t*, get_store_t*, set_store_interface_t*);
//typedef void (*tcp_serve_func)(std::iostream &, get_store_t*, set_store_interface_t*);

// Just for this dummy namespace interface thing until clustering is better prepared
#include "protocol/redis/redis.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/dummy_namespace_interface.hpp"
#include "protocol/redis/redis_proto.hpp"

/* Listens for TCP connections on the given port. Handles connections with appropriate protocol */
struct protocol_listener_t : public home_thread_mixin_t {

    // TODO Redis: Add extra argument for protocol if non-expected port number?
    protocol_listener_t(int port, get_store_t *get_store, set_store_interface_t *set_store);
    ~protocol_listener_t();

    void set_interface(namespace_interface_t<redis_protocol_t> *intface) {
        redis_interface = intface;
    }

private:
    get_store_t *get_store;
    set_store_interface_t *set_store;
    
    namespace_interface_t<redis_protocol_t> *redis_interface;

    cond_t pulse_to_begin_shutdown;
    drain_semaphore_t active_connection_drain_semaphore;
    int next_thread;
    boost::scoped_ptr<tcp_listener_t> tcp_listener;
    tcp_serve_func serve_func;

    void handle(boost::scoped_ptr<tcp_conn_t> &conn);
};

#endif /* __PROTOCOL_HPP__ */
