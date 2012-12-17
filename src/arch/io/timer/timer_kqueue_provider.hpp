// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef TIMER_KQUEUE_PROVIDER_HPP_
#define TIMER_KQUEUE_PROVIDER_HPP_

#include "arch/runtime/event_queue.hpp"

struct timer_provider_callback_t;

// This uses a kqueue to basically act like a Linux timerfd.  This isn't designed to run on kqueue.
struct timer_kqueue_provider_t : public linux_event_callback_t {
public:
    timer_kqueue_provider_t(linux_event_queue_t *queue,
                            timer_provider_callback_t *callback,
                            time_t secs, int32_t nsecs);
    ~timer_kqueue_provider_t();

private:
    void on_event(int events);

    linux_event_queue_t *queue_;
    timer_provider_callback_t *callback_;
    fd_t kq_fd_;

    DISABLE_COPYING(timer_kqueue_provider_t);
};


#endif  // TIMER_KQUEUE_PROVIDER_HPP_
