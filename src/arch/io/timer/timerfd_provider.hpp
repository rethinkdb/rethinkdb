// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_TIMER_TIMERFD_PROVIDER_HPP_
#define ARCH_IO_TIMER_TIMERFD_PROVIDER_HPP_

#include "arch/runtime/event_queue.hpp"

struct timer_provider_callback_t;

struct timerfd_provider_t : public linux_event_callback_t {
public:
    explicit timerfd_provider_t(event_queue_t *_queue);
    ~timerfd_provider_t();

    void schedule_oneshot(int64_t next_time_in_nanos, timer_provider_callback_t *cb);
    void unschedule_oneshot();

private:
    void on_event(int events);

    event_queue_t *queue;
    fd_t timer_fd;
    timer_provider_callback_t *callback;

    DISABLE_COPYING(timerfd_provider_t);
};

#endif // ARCH_IO_TIMER_TIMERFD_PROVIDER_HPP_

