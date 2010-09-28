#ifndef __ARCH_LINUX_ARCH_HPP__
#define __ARCH_LINUX_ARCH_HPP__

#include "arch/linux/io.hpp"
#include "arch/linux/event_queue.hpp"

struct linux_io_config_t {
    typedef linux_event_queue_t event_queue_t;
    typedef linux_direct_file_t direct_file_t;
    typedef linux_net_listener_t net_listener_t;
    typedef linux_net_conn_t net_conn_t;
    typedef linux_iocallback_t iocallback_t;
    typedef linux_net_conn_callback_t net_conn_callback_t;
};

#endif /* __ARCH_LINUX_ARCH_HPP__ */
