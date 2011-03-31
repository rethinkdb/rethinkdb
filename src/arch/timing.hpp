#ifndef __ARCH_TIMING_HPP__
#define __ARCH_TIMING_HPP__

#include "concurrency/cond_var.hpp"

/* Coroutine function that delays for some number of milliseconds. */

void nap(int ms);

/* Call pulse_after_time() to pulse a multicond later. If the multicond is pulsed
by something else before the timeout elapses, then the timer started by pulse_after_time()
is automatically cancelled. */

void pulse_after_time(multicond_t *mc, int ms);

#endif /* __ARCH_TIMING_HPP__ */
