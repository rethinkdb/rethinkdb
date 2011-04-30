#ifndef __ARCH_MOCK_ARCH_HPP__
#define __ARCH_MOCK_ARCH_HPP__

#include "arch/mock/io.hpp"

/* The "mock" architecture is a replacement IO layer that sits on top of the actual
IO layer. It introduces arbitrary delays in the IO calls in order to expose bugs
in the code that uses the IO layer. */

template<class inner_io_config_t>
struct mock_io_config_t {
    typedef typename inner_io_config_t::thread_pool_t thread_pool_t;

    typedef typename inner_io_config_t::io_backend_t io_backend_t;
    typedef mock_file_t<inner_io_config_t> file_t;
    typedef mock_direct_file_t<inner_io_config_t> direct_file_t;
    typedef mock_nondirect_file_t<inner_io_config_t> nondirect_file_t;
    typedef mock_iocallback_t iocallback_t;
    
    typedef typename inner_io_config_t::tcp_listener_t tcp_listener_t;
    typedef typename inner_io_config_t::tcp_listener_callback_t tcp_listener_callback_t;
    
    typedef typename inner_io_config_t::tcp_conn_t tcp_conn_t;
    
    static timer_token_t *add_timer(long ms, void (*callback)(void *), void *ctx) {
        return inner_io_config_t::add_timer(ms, callback, ctx);
    }
    static timer_token_t *fire_timer_once(long ms, void (*callback)(void *), void *ctx) {
        return inner_io_config_t::fire_timer_once(ms, callback, ctx);
    }
    static void cancel_timer(timer_token_t *timer) {
        return inner_io_config_t::cancel_timer(timer);
    }
};

#endif /* __ARCH_MOCK_ARCH_HPP__ */
