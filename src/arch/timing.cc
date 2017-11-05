// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "arch/timing.hpp"

#include <algorithm>
#include <functional>

#include "arch/arch.hpp"
#include "arch/runtime/coroutines.hpp"
#include "concurrency/wait_any.hpp"

// nap()

void nap(int64_t ms) THROWS_NOTHING {
    if (ms > 0) {
        signal_timer_t timer;
        timer.start(ms);
        timer.wait_lazily_ordered();
    }
}

void nap(int64_t ms, const signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    signal_timer_t timer(ms);
    wait_interruptible(&timer, interruptor);
}

// signal_timer_t

signal_timer_t::signal_timer_t() : timer(nullptr) { }
signal_timer_t::signal_timer_t(int64_t ms) : timer(nullptr) {
    start(ms);
}

signal_timer_t::~signal_timer_t() {
    if (timer != nullptr) {
        cancel_timer(timer);
    }
}

void signal_timer_t::start(int64_t ms) {
    guarantee(timer == nullptr);
    guarantee(!is_pulsed());
    if (ms == 0) {
        pulse();
    } else {
        guarantee(ms > 0);
        timer = fire_timer_once(ms, this);
    }
}

bool signal_timer_t::cancel() {
    if (timer != nullptr) {
        cancel_timer(timer);
        timer = nullptr;
        return true;
    }
    return false;
}

bool signal_timer_t::is_running() const {
    return is_pulsed() || timer != nullptr;
}

void signal_timer_t::on_timer(ticks_t) {
    timer = nullptr;
    pulse();
}

// repeating_timer_t

repeating_timer_t::repeating_timer_t(
        int64_t interval_ms, const std::function<void()> &_ringee) :
    interval(interval_ms),
    last_ticks(get_ticks()),
    expected_next_ticks(ticks_t{last_ticks.nanos + interval * MILLION}),
    ringee(_ringee) {
    rassert(interval_ms > 0);
    timer = add_timer(interval_ms, this);
}

repeating_timer_t::repeating_timer_t(
        int64_t interval_ms, repeating_timer_callback_t *_cb) :
    interval(interval_ms),
    last_ticks(get_ticks()),
    expected_next_ticks(ticks_t{last_ticks.nanos + interval * MILLION}),
    ringee([_cb]() { _cb->on_ring(); }) {
    rassert(interval_ms > 0);
    timer = add_timer(interval_ms, this);
}

repeating_timer_t::~repeating_timer_t() {
    cancel_timer(timer);
}

void repeating_timer_t::change_interval(int64_t interval_ms) {
    if (interval_ms == interval) {
        return;
    }

    interval = interval_ms;
    cancel_timer(timer);
    expected_next_ticks.nanos = std::min<int64_t>(last_ticks.nanos + interval_ms * MILLION,
                                                  expected_next_ticks.nanos);
    timer = add_timer2(expected_next_ticks, interval_ms, this);
}

void repeating_timer_t::clamp_next_ring(int64_t delay_ms) {
    int64_t t = last_ticks.nanos + delay_ms * MILLION;
    if (t < expected_next_ticks.nanos) {
        cancel_timer(timer);
        expected_next_ticks = ticks_t{t};
        timer = add_timer2(expected_next_ticks, interval, this);
    }
}

void call_ringer(std::function<void()> ringee) {
    // It would be very easy for a user of repeating_timer_t to have
    // object lifetime errors, if their ring function blocks.  So we
    // have this assertion.
    ASSERT_FINITE_CORO_WAITING;
    ringee();
}

void repeating_timer_t::on_timer(ticks_t ticks) {
    // Spawn _now_, otherwise the repeating_timer_t lifetime might end
    // before ring gets used.
    last_ticks = ticks;
    expected_next_ticks.nanos = last_ticks.nanos + interval * MILLION;
    coro_t::spawn_now_dangerously(std::bind(call_ringer, ringee));
}
