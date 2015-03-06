// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_PUMP_CORO_HPP_
#define CONCURRENCY_PUMP_CORO_HPP_

#include <functional>

#include "concurrency/auto_drainer.hpp"
#include "concurrency/mutex_assertion.hpp"

/* `pump_coro_t` is a class for managing a blocking function that must eventually run
after a certain event occurs; but if the event occurs multiple times in quick succession,
it's OK for the function to only run once. For example, we might want to flush a value to
disk every time it changes.

Use it as follows: Construct a `pump_coro_t` with the callback you want to run. Whenever
you call `notify()`, the callback will be run in a coroutine within some reasonable
amount of time. The callback must call `ack()` at least once. If another call to
`notify()` comes in before the call to `ack()`, the callback will not be run again; but
if it comes in after the call to `ack()`, the callback will be run again. There will
never be multiple instances of the callback running at once. */

class pump_coro_t {
public:
    pump_coro_t(const std::function<void(signal_t *)> &callback);
    void notify();
    void ack();
private:
    void run(auto_drainer_t::lock_t keepalive);
    std::function<void(signal_t *)> const callback;
    mutex_assertion_t mutex;
    bool is_running, is_notified, is_acked;
    auto_drainer_t drainer;
};

#endif /* CONCURRENCY_PUMP_CORO_HPP_ */

