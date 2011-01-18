
#ifndef __ARCH_LINUX_TIMER_SIGNAL_PROVIDER_HPP__
#define __ARCH_LINUX_TIMER_SIGNAL_PROVIDER_HPP__

#include "timer_provider_callback.hpp"

#define TIMER_NOTIFY_SIGNAL    (SIGRTMIN + 3)

/* Kernel timer provider based on signals */
struct timer_signal_provider_t : public linux_event_callback_t {
public:
    timer_signal_provider_t(linux_event_queue_t *_queue,
                            timer_provider_callback_t *_callback,
                            time_t secs, long nsecs);
    ~timer_signal_provider_t();
    
    void on_event(int events);
    
private:
    linux_event_queue_t *queue;
    timer_provider_callback_t *callback;
    timer_t timerid;
    sigevent evp;    // notify event
};

#endif // __ARCH_LINUX_TIMER_SIGNAL_PROVIDER_HPP__

