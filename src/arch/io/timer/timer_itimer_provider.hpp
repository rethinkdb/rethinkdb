// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef TIMER_ITIMER_PROVIDER_HPP_
#define TIMER_ITIMER_PROVIDER_HPP_

#include "errors.hpp"
#include "arch/runtime/event_queue.hpp"

void timer_itimer_forward_alrm();

struct timer_provider_callback_t;

class timer_itimer_provider_t : public intrusive_list_node_t<timer_itimer_provider_t> {
public:
    timer_itimer_provider_t(linux_event_queue_t *queue,
                            timer_provider_callback_t *callback,
                            time_t secs, int32_t nsecs);
    ~timer_itimer_provider_t();

private:
    friend void timer_itimer_forward_alrm();
    void on_alrm();

    linux_event_queue_t *queue_;
    timer_provider_callback_t *callback_;

    DISABLE_COPYING(timer_itimer_provider_t);
};



#endif  // TIMER_ITIMER_PROVIDER_HPP_
