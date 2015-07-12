// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "concurrency/pump_coro.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/interruptor.hpp"
#include "containers/map_sentries.hpp"

pump_coro_t::pump_coro_t(
        const std::function<void(signal_t *)> &_callback,
        size_t _max_callbacks) :
    callback(_callback),
    max_callbacks(_max_callbacks),
    running(0), starting(false), queued(false),
    timestamp(0), drained(false), running_timestamp(nullptr)
{
    guarantee(max_callbacks >= 1);
}

void pump_coro_t::notify() {
    assert_thread();
    mutex_assertion_t::acq_t acq(&mutex);
    if (drained) {
        return;
    }
    ++timestamp;
    /* If `starting` is `true`, then there's already a coroutine that's going to call the
    callback soon, so we don't need to start another one */
    if (!starting) {
        if (running < max_callbacks) {
            starting = true;
            coro_t::spawn_sometime(std::bind(&pump_coro_t::run, this, drainer.lock()));
        } else {
            queued = true;
        }
    }
}

void pump_coro_t::include_latest_notifications() {
    assert_thread();
    mutex_assertion_t::acq_t acq(&mutex);
    guarantee(max_callbacks == 1, "Don't use `include_latest_notifications()` with "
        "max_callbacks > 1");
    guarantee(running_timestamp != nullptr, "`include_latest_notifications()` should "
        "only be called from within the callback.");
    *running_timestamp = timestamp;
    queued = false;
}

void pump_coro_t::flush(signal_t *interruptor) {
    assert_thread();
    mutex_assertion_t::acq_t acq(&mutex);
    guarantee(!drained, "flush() might never succeed if we're draining");
    if (running == 0 && !starting) {
        guarantee(!queued, "queued can't be true if running != max_callbacks");
        return;
    }
    cond_t cond;
    multimap_insertion_sentry_t<uint64_t, cond_t *>
        sentry(&flush_waiters, timestamp, &cond);
    acq.reset();
    wait_interruptible(&cond, interruptor);
}

void pump_coro_t::drain() {
    assert_thread();
    {
        mutex_assertion_t::acq_t acq(&mutex);
        drained = true;
    }
    drainer.drain();
}

void pump_coro_t::run(auto_drainer_t::lock_t keepalive) {
    mutex_assertion_t::acq_t acq(&mutex);
    guarantee(starting);
    guarantee(!queued);
    guarantee(running < max_callbacks);
    starting = false;
    ++running;

    while (!keepalive.get_drain_signal()->is_pulsed()) {
        uint64_t local_timestamp = timestamp;
        assignment_sentry_t<uint64_t *> timestamp_sentry;
        if (max_callbacks == 1) {
            timestamp_sentry.reset(&running_timestamp, &local_timestamp);
        }
        acq.reset();

        try {
            callback(keepalive.get_drain_signal());
        } catch (const interrupted_exc_t &) {
            return;
        }
        coro_t::yield();

        acq.reset(&mutex);
        for (auto it = flush_waiters.begin();
                it != flush_waiters.upper_bound(timestamp);
                ++it) {
            it->second->pulse_if_not_already_pulsed();
        }
        if (queued) {
            queued = false;
        } else {
            break;
        }
    }
    
    --running;
}

