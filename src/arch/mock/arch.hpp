#ifndef __ARCH_MOCK_ARCH_HPP__
#define __ARCH_MOCK_ARCH_HPP__

#include "arch/mock/io.hpp"

template<class inner_io_config_t>
struct mock_io_config_t {

    typedef typename inner_io_config_t::event_queue_t event_queue_t;
    
    typedef mock_direct_file_t<inner_io_config_t> direct_file_t;
    typedef mock_iocallback_t iocallback_t;
    
    typedef typename inner_io_config_t::net_listener_t net_listener_t;
    typedef typename inner_io_config_t::net_conn_t net_conn_t;
    typedef typename inner_io_config_t::net_conn_callback_t net_conn_callback_t;
};

#endif /* __ARCH_MOCK_ARCH_HPP__ */
