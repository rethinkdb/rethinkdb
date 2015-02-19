// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "concurrency/min_version_enforcer.hpp"

#include "concurrency/signal.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/interruptor.hpp"
#include "concurrency/wait_any.hpp"

void min_version_enforcer_t::bump_version(state_timestamp_t new_ts) {
    assert_thread();
    if (new_ts > current_timestamp) {
        current_timestamp = new_ts;
        internal_pump();
    }
}

void min_version_enforcer_t::wait(min_version_token_t token)
        THROWS_ONLY(interrupted_exc_t) {
    cond_t dummy_interruptor;
    wait_interruptible(token, &dummy_interruptor);
}

void min_version_enforcer_t::wait_interruptible(
        min_version_token_t token,
        const signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    assert_thread();

    // Can we proceed immediately?
    if (token.min_timestamp <= current_timestamp) {
        return;
    } else {
        // Put a waiter on the queue and wait for it to be pulsed
        auto drainer_lock = drainer.lock();
        internal_waiter_t waiter(token);
        waiter_queue.push(&waiter);

        try {
            wait_any_t any_interruptor(interruptor, drainer_lock.get_drain_signal());
            ::wait_interruptible(&waiter.on_runnable, &any_interruptor);
        } catch (const interrupted_exc_t &) {
            // Remove waiter from the queue
            waiter_queue.remove(&waiter);
            throw;
        }
    }
}

void min_version_enforcer_t::internal_pump() THROWS_NOTHING {
    assert_thread();
    ASSERT_FINITE_CORO_WAITING;
    if (!in_pump) {
        in_pump = true;
        while (!waiter_queue.empty() &&
               waiter_queue.peek()->token.min_timestamp <= current_timestamp) {
            internal_waiter_t *waiter = waiter_queue.pop();
            waiter->on_runnable.pulse();
        }
        in_pump = false;
    }
}