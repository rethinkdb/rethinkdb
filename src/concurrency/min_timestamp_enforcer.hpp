// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_MIN_TIMESTAMP_ENFORCER_HPP_
#define CONCURRENCY_MIN_TIMESTAMP_ENFORCER_HPP_

#include "concurrency/auto_drainer.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/interruptor.hpp"
#include "concurrency/signal.hpp"
#include "containers/intrusive_priority_queue.hpp"
#include "rpc/serialize_macros.hpp"
#include "timestamps.hpp"

/* `min_timestamp_enforcer_t` can be used to make sure that certain operations
are put on hold until some entity has reached at least a certain timestamp.
A common use case is to make sure that reads are not run before a certain
(set of) write(s) has been performed.
Similar to the fifo enforcer, `min_timestamp_enforcer_t` works based on tokens
that you can send over the network. */

class min_timestamp_token_t {
public:
    min_timestamp_token_t() THROWS_NOTHING { }
    explicit min_timestamp_token_t(state_timestamp_t t) THROWS_NOTHING :
        min_timestamp(t) { }
    state_timestamp_t min_timestamp;
};

RDB_MAKE_SERIALIZABLE_1(min_timestamp_token_t, min_timestamp);

class min_timestamp_enforcer_t : public home_thread_mixin_debug_only_t {
public:

    min_timestamp_enforcer_t()
        : current_timestamp(state_timestamp_t::zero()) { }

    explicit min_timestamp_enforcer_t(state_timestamp_t ts)
        : current_timestamp(ts) { }

    /* All reads that are waiting on a timestamp <= `new_ts` can now pass. */
    void bump_timestamp(state_timestamp_t new_ts);

    /* Blocks until the desired version has been reached or the
    min_timestamp_enforcer_t is destroyed (in that case it throws an
    interrupted_exc_t. */
    void wait(min_timestamp_token_t token) THROWS_ONLY(interrupted_exc_t);
    void wait_interruptible(min_timestamp_token_t token, const signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

private:
    void internal_pump() THROWS_NOTHING;

    /* The current version */
    state_timestamp_t current_timestamp;

    /* `internal_waiter_t` represents an operation that waits in line to
    go through the min_version enforcer. */
    struct internal_waiter_t
            : public intrusive_priority_queue_node_t<internal_waiter_t> {
    public:
        explicit internal_waiter_t(min_timestamp_token_t t)
            : token(t) { }

        /* Pulsed when the operation is good to go*/
        cond_t on_runnable;

        /* The token of the operation */
        min_timestamp_token_t token;

        friend bool left_is_higher_priority(
                const min_timestamp_enforcer_t::internal_waiter_t *left,
                const min_timestamp_enforcer_t::internal_waiter_t *right) {
            return left->token.min_timestamp < right->token.min_timestamp;
        }
    };

    intrusive_priority_queue_t<internal_waiter_t> waiter_queue;

    auto_drainer_t drainer;

    DISABLE_COPYING(min_timestamp_enforcer_t);
};


#endif /* CONCURRENCY_MIN_TIMESTAMP_ENFORCER_HPP_ */
