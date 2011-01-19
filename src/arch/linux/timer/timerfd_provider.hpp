
#ifndef __ARCH_LINUX_TIMERFD_PROVIDER_HPP__
#define __ARCH_LINUX_TIMERFD_PROVIDER_HPP__

#include "timer_provider_callback.hpp"

/* Kernel timer provider based on timerfd  */
struct timerfd_provider_t : public linux_event_callback_t {
public:
    timerfd_provider_t(linux_event_queue_t *_queue,
                       timer_provider_callback_t *_callback,
                       time_t secs, long nsecs);
    ~timerfd_provider_t();
    
    void on_event(int events);
    
private:
    linux_event_queue_t *queue;
    timer_provider_callback_t *callback;
    fd_t timer_fd;
};

#endif // __ARCH_LINUX_TIMERFD_PROVIDER_HPP__

