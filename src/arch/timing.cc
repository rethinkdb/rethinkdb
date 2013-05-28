// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/timing.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "arch/runtime/coroutines.hpp"
#include "concurrency/wait_any.hpp"

// nap()

void nap(int64_t ms) THROWS_NOTHING {
    if (ms > 0) {
        signal_timer_t timer(ms);
        timer.wait_lazily_ordered();
    }
}

void nap(int64_t ms, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    signal_timer_t timer(ms);
    wait_interruptible(&timer, interruptor);
}

// signal_timer_t

signal_timer_t::signal_timer_t(int64_t ms) : timer(NULL) {
    if (ms == 0) {
        pulse();
    } else {
        guarantee(ms > 0);
        timer = fire_timer_once(ms, this);
    }
}

signal_timer_t::~signal_timer_t() {
    if (timer) cancel_timer(timer);
}

void signal_timer_t::on_timer() {
    timer = NULL;
    pulse();
}

// repeating_timer_t

repeating_timer_t::repeating_timer_t(int64_t frequency_ms, repeating_timer_callback_t *_ringee) :
    ringee(_ringee) {
    rassert(frequency_ms > 0);
    timer = add_timer(frequency_ms, this);
}

repeating_timer_t::~repeating_timer_t() {
    cancel_timer(timer);
}

void call_ringer(repeating_timer_callback_t *ringee) {
    // It would be very easy for a user of repeating_timer_t to have
    // object lifetime errors, if their ring function blocks.  So we
    // have this assertion.
    ASSERT_FINITE_CORO_WAITING;
    ringee->on_ring();
}

void repeating_timer_t::on_timer() {
    // Spawn _now_, otherwise the reating_timer_t lifetime might end
    // before ring gets used.
    coro_t::spawn_now_dangerously(boost::bind(call_ringer, ringee));
}
