// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "concurrency/pump_coro.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/interruptor.hpp"
#include "containers/map_sentries.hpp"

pump_coro_t::pump_coro_t(const std::function<void(signal_t *)> &_callback) :
    callback(_callback), is_running(false), is_notified(false) { }

void pump_coro_t::notify() {
    mutex_assertion_t::acq_t acq(&mutex);
    is_notified = true;
    if (!is_running) {
        is_running = true;
        coro_t::spawn_sometime(std::bind(&pump_coro_t::run, this, drainer.lock()));
    }
}

void pump_coro_t::include_latest_notifications() {
    mutex_assertion_t::acq_t acq(&mutex);
    is_notified = false;
    for (auto &&pair : flush_waiters) {
        pair.second = true;
    }
}

void pump_coro_t::flush(signal_t *interruptor) {
    cond_t cond;
    map_insertion_sentry_t<cond_t *, bool> sentry(&flush_waiters, &cond, false);
    wait_interruptible(&cond, interruptor);
}

void pump_coro_t::run(auto_drainer_t::lock_t keepalive) {
    try {
        mutex_assertion_t::acq_t acq(&mutex);
        guarantee(is_running);
        guarantee(is_notified);
        while (is_notified && !keepalive.get_drain_signal()->is_pulsed()) {
            is_notified = false;
            for (auto &&pair : flush_waiters) {
                pair.second = true;
            }
            acq.reset();

            callback(keepalive.get_drain_signal());

            acq.reset(&mutex);
            for (const auto &pair : flush_waiters) {
                /* We test `pair.second` so that we don't notify flush waiters that
                started waiting after the callback started. */
                if (pair.second) {
                    pair.first->pulse_if_not_already_pulsed();
                }
            }
        }
        is_running = false;
    } catch (const interrupted_exc_t &) {
        guarantee(keepalive.get_drain_signal()->is_pulsed());
    }
}
