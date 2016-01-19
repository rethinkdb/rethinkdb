// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_IO_TIMER_TIMER_KQUEUE_PROVIDER_HPP_
#define ARCH_IO_TIMER_TIMER_KQUEUE_PROVIDER_HPP_

#include "arch/runtime/event_queue.hpp"

struct timer_provider_callback_t;

// This uses a kqueue to basically act like a Linux timerfd.  This isn't designed to run on kqueue.
struct timer_kqueue_provider_t : public linux_event_callback_t {
public:
    explicit timer_kqueue_provider_t(linux_event_queue_t *queue);
    ~timer_kqueue_provider_t();

    void schedule_oneshot(int64_t next_time_in_nanos, timer_provider_callback_t *cb);
    void unschedule_oneshot();

private:
    void on_event(int events);

    linux_event_queue_t *queue_;
    fd_t kq_fd_;
    timer_provider_callback_t *callback_;

    DISABLE_COPYING(timer_kqueue_provider_t);
};


#endif  // ARCH_IO_TIMER_TIMER_KQUEUE_PROVIDER_HPP_
