// Copyright 2015 RethinkDB, all rights reserved.
#ifndef ARCH_IO_TIMER_TIMER_EVPORT_PROVIDER_HPP_
#define ARCH_IO_TIMER_TIMER_EVPORT_PROVIDER_HPP_

#include "arch/runtime/event_queue.hpp"

struct timer_provider_callback_t;

struct timer_evport_provider_t : public linux_event_callback_t {
public:
    explicit timer_evport_provider_t(linux_event_queue_t *_queue);
    ~timer_evport_provider_t();

    void schedule_oneshot(int64_t next_time_in_nanos, timer_provider_callback_t *cb);
    void unschedule_oneshot();

private:
    void on_event(int events);

    linux_event_queue_t *queue;
    timer_t timerid;
    timer_provider_callback_t *callback;

    DISABLE_COPYING(timer_evport_provider_t);
};

#endif  // ARCH_IO_TIMER_TIMER_EVPORT_PROVIDER_HPP_
