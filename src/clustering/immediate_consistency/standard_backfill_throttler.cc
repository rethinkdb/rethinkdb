// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/standard_backfill_throttler.hpp"

#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/wait_any.hpp"

static const size_t max_active_backfills = 8;

standard_backfill_throttler_t::~standard_backfill_throttler_t() {
    guarantee(active.empty());
    guarantee(waiting.empty());
}

void standard_backfill_throttler_t::enter(lock_t *lock, signal_t *interruptor_on_lock) {
    cross_thread_signal_t interruptor_on_home(interruptor_on_lock, home_thread());
    on_thread_t thread_switcher(home_thread());
    scoped_ptr_t<new_mutex_acq_t> mutex_acq(
        new new_mutex_acq_t(&mutex, &interruptor_on_home));

    if (active.size() < max_active_backfills) {
        /* There is no contention, so we can start right away */
        active.insert(std::make_pair(lock->priority, lock));

    } else {
        /* Insert `cond` into the waiting queue. */
        cond_t cond;
        auto it = waiting.insert(std::make_pair(
            lock->priority, std::make_pair(lock, &cond)));

        /* If there's a lower-priority backfill that's already active, preempt it. This
        only pulses the preempt signal; it doesn't immediately remove it from `active`.
        */
        auto w_it = waiting.rbegin();
        auto a_it = active.begin();
        while (w_it != waiting.rend() && a_it != active.end()
                && a_it->first < w_it->first) {
            {
                on_thread_t thread_switcher_2(a_it->second->home_thread());
                if (!a_it->second->get_preempt_signal()->is_pulsed()) {
                    preempt(a_it->second);
                }
            }
            ++w_it;
            ++a_it;
        }

        /* Release the mutex before we wait for the cond */
        mutex_acq.reset();

        /* Wait until it's our turn to go. Note that we don't throw `interrupted_exc_t`
        because we still need to clean up. */
        wait_any_t waiter(&cond, &interruptor_on_home);
        waiter.wait_lazily_unordered();

        mutex_acq.init(new new_mutex_acq_t(&mutex));   /* again, not interruptible */
        ASSERT_NO_CORO_WAITING;
        if (cond.is_pulsed()) {
            guarantee(active.count(std::make_pair(lock->priority, lock)) == 1);
        } else {
            waiting.erase(it);
            throw interrupted_exc_t();
        }
    }
}

void standard_backfill_throttler_t::exit(lock_t *lock) {
    on_thread_t thread_switcher(home_thread());
    new_mutex_acq_t acq(&mutex);
    ASSERT_NO_CORO_WAITING;

    /* Find the entry corresponding to this particular backfill. */
    auto it = active.find(std::make_pair(lock->priority, lock));
    guarantee(it != active.end());
    active.erase(it);

    /* Find the highest-priority backfill that's waiting to start */
    auto jt = waiting.end();
    if (jt != waiting.begin()) {
        --jt;

        /* Pulse the `cond_t` so that `enter()` can return */
        jt->second.second->pulse();

        /* Transfer the backfill from `active` to `waiting` */
        active.insert(std::make_pair(jt->first, jt->second.first));
        waiting.erase(jt);
    }
}

