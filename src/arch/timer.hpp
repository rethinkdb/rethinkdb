// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_TIMER_HPP_
#define ARCH_TIMER_HPP_

#include "containers/intrusive_priority_queue.hpp"
#include "arch/io/timer_provider.hpp"

class timer_token_t;

struct timer_callback_t {
    virtual void on_timer() = 0;
    virtual ~timer_callback_t() { }
};

class timer_token_t : public intrusive_priority_queue_node_t<timer_token_t> {
    friend class timer_handler_t;

    bool is_higher_priority_than(timer_token_t *competitor) {
        return next_time_in_nanos < competitor->next_time_in_nanos;
    }

private:
    timer_token_t() : interval_nanos(-1), next_time_in_nanos(-1), callback(NULL) { }

    // The time between rings, if a repeating timer, otherwise zero.
    int64_t interval_nanos;

    // The time of the next 'ring'.
    int64_t next_time_in_nanos;

    // The callback we call upon each 'ring'.
    timer_callback_t *callback;

    DISABLE_COPYING(timer_token_t);
};

/* This timer class uses the underlying OS timer provider to get one-shot timing events. It then
 * manages a list of application timers based on that lower level interface. Everyone who needs a
 * timer should use this class (through the thread pool). */
class timer_handler_t : private timer_provider_interactor_t {
public:
    explicit timer_handler_t(linux_event_queue_t *queue);
    ~timer_handler_t();

    timer_token_t *add_timer_internal(int64_t ms, timer_callback_t *callback, bool once);
    void cancel_timer(timer_token_t *timer);

private:
    void on_oneshot();

    // The timer provider, a platform-dependent typedef for interfacing with the OS.
    timer_provider_t timer_provider;

    // The expected time of the next on_oneshot call.  If the oneshot arrived earlier than this
    // time, we pretend that it had arrived on time.
    int64_t expected_oneshot_time_in_nanos;

    // A priority queue of timer tokens, ordered by the soonest.
    intrusive_priority_queue_t<timer_token_t> token_queue;

    DISABLE_COPYING(timer_handler_t);
};

#endif /* ARCH_TIMER_HPP_ */
