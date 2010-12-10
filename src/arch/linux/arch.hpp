#ifndef __ARCH_LINUX_ARCH_HPP__
#define __ARCH_LINUX_ARCH_HPP__

#include "arch/linux/disk.hpp"
#include "arch/linux/network.hpp"
#include "arch/linux/event_queue.hpp"
#include "arch/linux/thread_pool.hpp"

struct linux_io_config_t {
    typedef linux_thread_pool_t thread_pool_t;
    
    typedef linux_direct_file_t direct_file_t;
    typedef linux_iocallback_t iocallback_t;
    
    typedef linux_net_listener_t net_listener_t;
    typedef linux_net_listener_callback_t net_listener_callback_t;
    
    typedef linux_net_conn_t net_conn_t;
    typedef linux_net_conn_read_callback_t net_conn_read_callback_t;
    typedef linux_net_conn_write_callback_t net_conn_write_callback_t;
    
    typedef linux_oldstyle_net_conn_t oldstyle_net_conn_t;
    typedef linux_oldstyle_net_conn_callback_t oldstyle_net_conn_callback_t;
    
    typedef linux_cpu_message_t cpu_message_t;
    
    static int get_cpu_id() {
        return linux_thread_pool_t::cpu_id;
    }
    
    static int get_num_cpus() {
        return linux_thread_pool_t::thread_pool->n_threads;
    }
    
    static bool continue_on_cpu(int cpu, linux_cpu_message_t *msg) {
        if (cpu == get_cpu_id()) {
            // The CPU to continue on is the CPU we are already on
            return true;
        } else {
            linux_thread_pool_t::thread->message_hub.store_message(cpu, msg);
            return false;
        }
    }
    
    static void call_later_on_this_cpu(linux_cpu_message_t *msg) {
        linux_thread_pool_t::thread->message_hub.store_message(get_cpu_id(), msg);
    }
    
    typedef linux_timer_token_t timer_token_t;
    
    static timer_token_t *add_timer(long ms, void (*callback)(void *), void *ctx) {
        return linux_thread_pool_t::thread->timer_handler.add_timer_internal(ms, callback, ctx, false);
    }
    
    static timer_token_t *fire_timer_once(long ms, void (*callback)(void *), void *ctx) {
        return linux_thread_pool_t::thread->timer_handler.add_timer_internal(ms, callback, ctx, true);
    }
    
    static void cancel_timer(timer_token_t *timer) {
        linux_thread_pool_t::thread->timer_handler.cancel_timer(timer);
    }
};

#endif /* __ARCH_LINUX_ARCH_HPP__ */
