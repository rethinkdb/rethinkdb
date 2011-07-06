#ifndef __ARCH_TIMING_HPP__
#define __ARCH_TIMING_HPP__

#include "concurrency/signal.hpp"

/* Coroutine function that delays for some number of milliseconds. */

void nap(int ms);

class timer_token_t;

/* Construct a `signal_timer_t` to start a one-shot timer. When the timer "rings",
the signal will be pulsed. It is safe to destroy the `signal_timer_t` before the
timer "rings". */

struct signal_timer_t : public signal_t {

    signal_timer_t(int ms);
    ~signal_timer_t();

private:
    static void on_timer_ring(void *v_timer);
    timer_token_t *timer;
};

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
