// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef TIMER_ITIMER_PROVIDER_HPP_
#define TIMER_ITIMER_PROVIDER_HPP_

#include "errors.hpp"
#include "arch/runtime/event_queue.hpp"

struct timer_provider_callback_t;

class timer_itimer_provider_t : public linux_event_callback_t {
public:
    timer_itimer_provider_t(linux_event_queue_t *queue,
                            timer_provider_callback_t *callback,
                            time_t secs, int32_t nsecs);
    ~timer_itimer_provider_t();

private:
    void on_event(int events);

    linux_event_queue_t *queue_;
    timer_provider_callback_t *callback_;

    DISABLE_COPYING(timer_itimer_provider_t);
};



#endif  // TIMER_ITIMER_PROVIDER_HPP_
