// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_TIMER_TIMER_WINDOWS_PROVIDER_HPP_
#define ARCH_IO_TIMER_TIMER_WINDOWS_PROVIDER_HPP_

#include "arch/runtime/event_queue.hpp"

struct timer_provider_callback_t;

// On Windows, the timer's resolution corresponds to
// the resolution of the system clock, typically around 15 ms.

struct timer_windows_provider_t {
public:
    explicit timer_windows_provider_t(iocp_event_queue_t *);
    ~timer_windows_provider_t();

    void schedule_oneshot(const int64_t next_time_in_nanos, timer_provider_callback_t *const cb);
    void unschedule_oneshot();

private:
    timer_provider_callback_t *callback;
    iocp_event_queue_t *event_queue;
    DISABLE_COPYING(timer_windows_provider_t);
};

#endif // ARCH_IO_TIMER_TIMER_WINDOWS_PROVIDER_HPP_

