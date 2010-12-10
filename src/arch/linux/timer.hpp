#ifndef __ARCH_LINUX_TIMER_HPP__
#define __ARCH_LINUX_TIMER_HPP__

#include "containers/intrusive_list.hpp"

struct linux_timer_token_t :
    public intrusive_list_node_t<linux_timer_token_t>
{
    
    friend class linux_timer_handler_t;
    
private:
    bool once;   // If 'false', the timer is repeating
    long interval_ms;   // If a repeating timer, this is the time between 'rings'
    long next_time_in_ms;   // This is the time (in ms since the server started) of the next 'ring'
    
    // It's unsafe to remove arbitrary timers from the list as we iterate over
    // it, so instead we set the 'deleted' flag and then remove them in a
    // controlled fashion.
    bool deleted;
    
    void (*callback)(void *ctx);
    void *context;
};


struct linux_timer_handler_t :
    public linux_event_callback_t
{
    explicit linux_timer_handler_t(linux_event_queue_t *queue);
    ~linux_timer_handler_t();
    
    linux_event_queue_t *queue;
    fd_t timer_fd;
    long timer_ticks_since_server_startup;
    intrusive_list_t<linux_timer_token_t> timers;
    
    void on_event(int events);
    
    linux_timer_token_t *add_timer_internal(long ms, void (*callback)(void *ctx), void *ctx, bool once);
    void cancel_timer(linux_timer_token_t *timer);
};

#endif /* __ARCH_LINUX_TIMER_HPP__ */
