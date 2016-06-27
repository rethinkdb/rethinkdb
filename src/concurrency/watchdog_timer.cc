// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "concurrency/watchdog_timer.hpp"

#include "arch/runtime/coroutines.hpp"
#include "arch/timing.hpp"
#include "random.hpp"
#include "utils.hpp"

watchdog_timer_t::blocker_t::blocker_t(watchdog_timer_t *p) : parent(p) {
    parent->assert_thread();
    ++parent->num_blockers;
    parent->state = watchdog_timer_t::state_t::NOT_TRIGGERED;
}

watchdog_timer_t::blocker_t::~blocker_t() {
    parent->assert_thread();
    guarantee(parent->num_blockers > 0);
    --parent->num_blockers;
    if (parent->num_blockers == 0) {
        parent->notify();
    }
}

watchdog_timer_t::watchdog_timer_t(
        int _min, int _max, const std::function<void()> &_callback,
        state_t initial_state) :
    min_timeout_ms(_min), max_timeout_ms(_max), callback(_callback),
    num_blockers(0), state(initial_state)
{
    if (initial_state == state_t::TRIGGERED && static_cast<bool>(callback)) {
        callback();
    }
    set_next_threshold();
    coro_t::spawn_sometime(std::bind(&watchdog_timer_t::run, this, drainer.lock()));
}

watchdog_timer_t::~watchdog_timer_t() {
    guarantee(num_blockers == 0);
}

void watchdog_timer_t::notify() {
    assert_thread();
    state = state_t::NOT_TRIGGERED;
    set_next_threshold();
}

void watchdog_timer_t::run(auto_drainer_t::lock_t keepalive) {
    try {
        for (;;) {
            microtime_t now = current_microtime();
            if (now > next_threshold) {
                ASSERT_NO_CORO_WAITING;
                if (num_blockers == 0) {
                    state = state_t::TRIGGERED;
                    if (static_cast<bool>(callback)) {
                        callback();
                    }
                }
                /* Wait a bit before calling `callback()` again */
                set_next_threshold();
            }
            if (next_threshold > now + max_timeout_ms * 1000) {
                /* This can only happen if the system clock goes backwards. Rather than
                wait until the system clock catches up to its old value, we reset
                `next_threshold` to only `max_timeout_ms` in the future. */
                next_threshold = now + max_timeout_ms * 1000;
            }
            nap((next_threshold - now) / 1000, keepalive.get_drain_signal());
        }
    } catch (const interrupted_exc_t &) {
        /* `watchdog_timer_t` is being destroyed */
    }
}

void watchdog_timer_t::set_next_threshold() {
    int timeout_ms = min_timeout_ms + randint(max_timeout_ms - min_timeout_ms + 1);
    next_threshold = current_microtime() + timeout_ms * 1000;
}

