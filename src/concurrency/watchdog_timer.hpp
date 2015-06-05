// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_WATCHDOG_TIMER_HPP_
#define CONCURRENCY_WATCHDOG_TIMER_HPP_

#include <functional>

#include "concurrency/auto_drainer.hpp"
#include "concurrency/interruptor.hpp"
#include "threading.hpp"
#include "time.hpp"

/* `watchdog_timer_t` keeps track of how long it's been since the `notify()` method was
last called. If ever `notify()` is not called for a sufficiently long interval (randomly
chosen between `min_timeout_ms` and `max_timeout_ms`), the watchdog becomes "triggered":
it periodically calls its `callback` until it is notified or destroyed. */

class watchdog_timer_t : public home_thread_mixin_debug_only_t {
public:
    enum state_t { TRIGGERED, NOT_TRIGGERED };

    /* Creating a `blocker_t` is equivalent to continuously calling `notify()` until the
    `blocker_t` is destroyed. */
    class blocker_t {
    public:
        blocker_t(watchdog_timer_t *parent);
        ~blocker_t();
    private:
        watchdog_timer_t *parent;
    };

    watchdog_timer_t(
        int min_timeout_ms,
        int max_timeout_ms,
        const std::function<void()> &callback,
        state_t initial_state = state_t::NOT_TRIGGERED);
    ~watchdog_timer_t();

    void notify();

    state_t get_state() const {
        return state;
    }

private:
    void run(auto_drainer_t::lock_t);
    void set_next_threshold();
    int min_timeout_ms, max_timeout_ms;
    std::function<void()> callback;
    microtime_t next_threshold;
    int num_blockers;
    state_t state;
    
    auto_drainer_t drainer;
};

#endif /* CONCURRENCY_WATCHDOG_TIMER_HPP_ */

