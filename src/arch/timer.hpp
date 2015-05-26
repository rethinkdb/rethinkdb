// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_TIMER_HPP_
#define ARCH_TIMER_HPP_

#include "containers/intrusive_priority_queue.hpp"
#include "arch/io/timer_provider.hpp"

class timer_token_t;

struct timer_callback_t {
    virtual void on_timer() = 0;
    virtual ~timer_callback_t() { }
};

/* This timer class uses the underlying OS timer provider to get one-shot timing events. It then
 * manages a list of application timers based on that lower level interface. Everyone who needs a
 * timer should use this class (through the thread pool). */
class timer_handler_t : private timer_provider_callback_t {
public:
    explicit timer_handler_t(event_queue_t *queue);
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


/* Timer functions create (non-)periodic timers, callbacks for which are
 * executed on the same thread that they were created on. Thus, non-thread-safe
 * (but coroutine-safe) concurrency primitives can be used where appropriate.
 */
timer_token_t *add_timer(int64_t ms, timer_callback_t *callback);
timer_token_t *fire_timer_once(int64_t ms, timer_callback_t *callback);
void cancel_timer(timer_token_t *timer);


#endif /* ARCH_TIMER_HPP_ */
