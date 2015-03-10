// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "concurrency/pump_coro.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/interruptor.hpp"
#include "containers/map_sentries.hpp"

pump_coro_t::pump_coro_t(
        const std::function<void(signal_t *)> &_callback,
        size_t _max_callbacks) :
    callback(_callback), max_callbacks(_max_callbacks),
    running(0), starting(false), queued(false)
{
    guarantee(max_callbacks >= 1);
}

void pump_coro_t::notify() {
    assert_thread();
    mutex_assertion_t::acq_t acq(&mutex);
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
    guarantee(running_flush_waiters != nullptr, "`include_latest_notifications()` "
        "should only be called from within the callback.");
    running_flush_waiters->insert(
        running_flush_waiters->end(),
        std::make_move_iterator(flush_waiters.begin()),
        std::make_move_iterator(flush_waiters.end()));
    flush_waiters.clear();
    queued = false;
}

void pump_coro_t::flush(signal_t *interruptor) {
    assert_thread();
    if (running == 0 && !starting) {
        guarantee(!queued, "queued can't be true if running != max_callbacks");
        return;
    }
    counted_t<counted_cond_t> waiter = make_counted<counted_cond_t>();
    flush_waiters.push_back(waiter);
    wait_interruptible(waiter.get(), interruptor);
}

void pump_coro_t::run(auto_drainer_t::lock_t keepalive) {
    mutex_assertion_t::acq_t acq(&mutex);
    guarantee(starting);
    guarantee(!queued);
    guarantee(running < max_callbacks);
    starting = false;
    ++running;
    try {
        while (!keepalive.get_drain_signal()->is_pulsed()) {
            std::vector<counted_t<counted_cond_t> > local_waiters;
            std::swap(flush_waiters, local_waiters);
            assignment_sentry_t<std::vector<counted_t<counted_cond_t> > *>
                flush_waiters_sentry;
            if (max_callbacks == 1) {
                flush_waiters_sentry.reset(&running_flush_waiters, &local_waiters);
            }
            acq.reset();

            callback(keepalive.get_drain_signal());

            acq.reset(&mutex);
            for (const auto &cond : local_waiters) {
                cond->pulse();
            }
            if (queued) {
                queued = false;
            } else {
                break;
            }
        }
    } catch (const interrupted_exc_t &) {
        guarantee(keepalive.get_drain_signal()->is_pulsed());
    }
    --running;
}

