#ifndef __REDIS_SERVER_HPP__
#define __REDIS_SERVER_HPP__

#include "arch/arch.hpp"
#include <iostream>

// Just for this dummy namespace interface thing until clustering is better prepared
#include "redis/redis.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/dummy_namespace_interface.hpp"
#include "redis/redis_proto.hpp"

/* Listens for TCP connections on the given port. Handles connections with redis. */
struct redis_listener_t : public home_thread_mixin_t {

    redis_listener_t(int port);
    ~redis_listener_t();

private:
    namespace_interface_t<redis_protocol_t> *redis_interface;

    cond_t pulse_to_begin_shutdown;
    drain_semaphore_t active_connection_drain_semaphore;
    int next_thread;
    boost::scoped_ptr<tcp_listener_t> tcp_listener;

    void handle(boost::scoped_ptr<tcp_conn_t> &conn);
};

#endif /* __REDIS_SERVER_HPP__ */
