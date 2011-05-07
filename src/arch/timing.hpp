#ifndef __ARCH_TIMING_HPP__
#define __ARCH_TIMING_HPP__

#include "concurrency/signal.hpp"

/* Coroutine function that delays for some number of milliseconds. */

void nap(int ms);

/* Call call_with_delay() to schedule a function to be called later. If you pulse the given signal
before the time has elapsed, then the timer will be aborted. */

void call_with_delay(int delay_ms, const boost::function<void()> &fun, signal_t *aborter);

/* Construct a repeating_timer_t to start a repeating timer. It will call its function
when the timer "rings". */

struct repeating_timer_t {

    repeating_timer_t(int frequency_ms, boost::function<void(void)> ring);
    ~repeating_timer_t();

private:
    static void on_timer_ring(void *v_timer);
    timer_token_t *timer;
    boost::function<void(void)> ring;
};

#endif /* __ARCH_TIMING_HPP__ */
