#ifndef ARCH_TIMING_HPP_
#define ARCH_TIMING_HPP_

#include "concurrency/signal.hpp"

/* Coroutine function that delays for some number of milliseconds. */

void nap(int ms) THROWS_NOTHING;

/* This variant takes an interruptor, and throws `interrupted_exc_t` if the
interruptor is pulsed before the timeout is up */

void nap(int ms, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

class timer_token_t;

/* Construct a `signal_timer_t` to start a one-shot timer. When the timer "rings",
the signal will be pulsed. It is safe to destroy the `signal_timer_t` before the
timer "rings". */

struct signal_timer_t : public signal_t {

    explicit signal_timer_t(int ms);
    ~signal_timer_t();

private:
    static void on_timer_ring(void *v_timer);
    timer_token_t *timer;
};

/* Construct a repeating_timer_t to start a repeating timer. It will call its function
when the timer "rings". */

class repeating_timer_callback_t {
public:
    virtual void on_ring() = 0;
protected:
    ~repeating_timer_callback_t() { }
};

class repeating_timer_t {
public:

    repeating_timer_t(int frequency_ms, repeating_timer_callback_t *ringee);
    ~repeating_timer_t();

private:
    static void on_timer_ring(void *v_timer);
    timer_token_t *timer;
    repeating_timer_callback_t *ringee;

    DISABLE_COPYING(repeating_timer_t);
};

#endif /* ARCH_TIMING_HPP_ */
