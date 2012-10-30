// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_TIMER_TIMERFD_PROVIDER_HPP_
#define ARCH_IO_TIMER_TIMERFD_PROVIDER_HPP_

#include "arch/runtime/event_queue.hpp"

struct timer_provider_callback_t;

/* Kernel timer provider based on timerfd  */
struct timerfd_provider_t : public linux_event_callback_t {
public:
    timerfd_provider_t(linux_event_queue_t *_queue,
                       timer_provider_callback_t *_callback,
                       time_t secs, int32_t nsecs);
    ~timerfd_provider_t();

    void on_event(int events);

private:
    linux_event_queue_t *queue;
    timer_provider_callback_t *callback;
    fd_t timer_fd;
};

#endif // ARCH_IO_TIMER_TIMERFD_PROVIDER_HPP_

