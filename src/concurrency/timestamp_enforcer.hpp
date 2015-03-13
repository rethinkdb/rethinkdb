// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_TIMESTAMP_ENFORCER_HPP_
#define CONCURRENCY_TIMESTAMP_ENFORCER_HPP_

#include <map>
#include <set>

#include "concurrency/cond_var.hpp"
#include "timestamps.hpp"

class timestamp_enforcer_t {
public:
    timestamp_enforcer_t(state_timestamp_t initial);

    /* Blocks until all timestamps less than or equal to `goal` have been completed. */
    void wait_all_before(state_timestamp_t goal, signal_t *interruptor);

    /* Marks the given timestamp as completed. */
    void complete(state_timestamp_t completed);

private:
    /* `timestamp` is the latest timestamp such that all timestamps less than or equal to
    it have been completed. */
    state_timestamp_t timestamp;

    /* `future_completed` is a collection of timestamps which have been completed. They
    are always greater than `timestamp + 1`. */
    std::set<state_timestamp_t> future_completed;

    /* `waiters` is a collection of condition variables that should be pulsed when
    `timestamp` reaches their value. */
    std::multimap<state_timestamp_t, cond_t *> waiters;
};

#endif /* CONCURRENCY_TIMESTAMP_ENFORCER_HPP_ */

