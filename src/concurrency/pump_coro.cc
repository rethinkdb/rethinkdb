// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "concurrency/pump_coro.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/interruptor.hpp"

pump_coro_t::pump_coro_t(const std::function<void(signal_t *)> &_callback) :
    callback(_callback), is_running(false), is_notified(false), is_acked(false) { }

void pump_coro_t::notify() {
    mutex_assertion_t::acq_t acq(&mutex);
    is_notified = true;
    if (!is_running) {
        is_running = true;
        coro_t::spawn_sometime(std::bind(&pump_coro_t::run, this, drainer.lock()));
    }
}

void pump_coro_t::ack() {
    mutex_assertion_t::acq_t acq(&mutex);
    is_notified = false;
    is_acked = true;
}

void pump_coro_t::run(auto_drainer_t::lock_t keepalive) {
    try {
        mutex_assertion_t::acq_t acq(&mutex);
        guarantee(is_running);
        guarantee(is_notified);
        while (is_notified && !keepalive.get_drain_signal()->is_pulsed()) {
            is_acked = false;
            acq.reset();
            callback(keepalive.get_drain_signal());
            acq.reset(&mutex);
            guarantee(is_acked, "pump_coro_t callback must call ack()");
        }
        is_running = false;
    } catch (const interrupted_exc_t &) {
        guarantee(keepalive.get_drain_signal()->is_pulsed());
    }
}
