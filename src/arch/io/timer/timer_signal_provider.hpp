// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_TIMER_TIMER_SIGNAL_PROVIDER_HPP_
#define ARCH_IO_TIMER_TIMER_SIGNAL_PROVIDER_HPP_

#include "arch/runtime/event_queue.hpp"

struct timer_provider_callback_t;

#define TIMER_NOTIFY_SIGNAL    (SIGRTMIN + 3)

/* Kernel timer provider based on signals */
struct timer_signal_provider_t {
public:
    explicit timer_signal_provider_t(linux_event_queue_t *_queue);
    ~timer_signal_provider_t();

    void schedule_oneshot(int64_t next_time_in_nanos, timer_provider_callback_t *cb);
    void unschedule_oneshot();

private:
    friend void timer_signal_provider_signal_handler(UNUSED int signum, siginfo_t *siginfo, UNUSED void *uctx);

    timer_t timerid;
    timer_provider_callback_t *callback;

    DISABLE_COPYING(timer_signal_provider_t);
};

#endif  // ARCH_IO_TIMER_TIMER_SIGNAL_PROVIDER_HPP_
