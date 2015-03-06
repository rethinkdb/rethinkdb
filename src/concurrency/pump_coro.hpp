// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_PUMP_CORO_HPP_
#define CONCURRENCY_PUMP_CORO_HPP_

#include <functional>
#include <map>

#include "concurrency/auto_drainer.hpp"
#include "concurrency/mutex_assertion.hpp"

/* `pump_coro_t` is a class for managing a blocking function that must eventually run
after a certain event occurs; but if the event occurs multiple times in quick succession,
it's OK for the function to only run once. For example, we might want to flush a value to
disk every time it changes. */

class pump_coro_t {
public:
    /* The constructor specifies which callback should be called. The callback may block.
    If the `pump_coro_t` is destroyed its interruptor will be pulsed and it may throw
    `interrupted_exc_t`. There will never be multiple instances of the callback running
    at once. The callback is guaranteed to stop before the `pump_coro_t` destructor
    returns. */
    pump_coro_t(const std::function<void(signal_t *)> &callback);

    /* `notify()` schedules the callback to be run, unless it's already scheduled. */
    void notify();

    /* Normally, if `notify()` is called while the callback is running, then the callback
    will be called again after it returns. But if the callback calls
    `include_latest_notifications()`, all notifications up to that point will be
    considered to be handled by this instance of the callback. */
    void include_latest_notifications();

    /* `flush()` blocks until all the calls to `notify()` up to this point have been
    handled. */
    void flush(signal_t *interruptor);

private:
    void run(auto_drainer_t::lock_t keepalive);
    std::function<void(signal_t *)> const callback;
    mutex_assertion_t mutex;
    bool is_running, is_notified;
    std::map<cond_t *, bool> flush_waiters;
    auto_drainer_t drainer;
};

#endif /* CONCURRENCY_PUMP_CORO_HPP_ */

