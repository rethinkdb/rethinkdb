// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "concurrency/timestamp_enforcer.hpp"

#include "concurrency/interruptor.hpp"
#include "containers/map_sentries.hpp"

void timestamp_enforcer_t::wait_all_before(
        state_timestamp_t goal, signal_t *interruptor) {
    if (goal > timestamp) {
        cond_t waiter;
        multimap_insertion_sentry_t<state_timestamp_t, cond_t *> sentry(
            &waiters, goal, &waiter);
        wait_interruptible(&waiter, interruptor);
        guarantee(timestamp >= goal);
    }
}

void timestamp_enforcer_t::complete(state_timestamp_t completed) {
    guarantee(completed > timestamp);
    guarantee(future_completed.count(completed) == 0);
    if (completed == timestamp.next()) {
        timestamp = completed;
        while (!future_completed.empty() &&
                *future_completed.begin() == timestamp.next()) {
            timestamp = timestamp.next();
            future_completed.erase(future_completed.begin());
        }
        for (auto it = waiters.begin(); it != waiters.upper_bound(timestamp); ++it) {
            it->second->pulse_if_not_already_pulsed();
        }
    } else {
        future_completed.insert(completed);
    }
}
