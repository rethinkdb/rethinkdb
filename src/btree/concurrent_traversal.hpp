// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_CONCURRENT_TRAVERSAL_HPP_
#define BTREE_CONCURRENT_TRAVERSAL_HPP_

#include "btree/depth_first_traversal.hpp"
#include "concurrency/interruptor.hpp"

class concurrent_traversal_adapter_t;

namespace profile { class trace_t; }

class concurrent_traversal_fifo_enforcer_signal_t {
public:
    void wait_interruptible() THROWS_ONLY(interrupted_exc_t);

private:
    friend class concurrent_traversal_adapter_t;

    concurrent_traversal_fifo_enforcer_signal_t(signal_t *eval_exclusivity_signal,
                                                concurrent_traversal_adapter_t *parent);

    signal_t *const eval_exclusivity_signal_;
    concurrent_traversal_adapter_t *const parent_;
};

class concurrent_traversal_callback_t {
public:
    concurrent_traversal_callback_t() { }

    /* See `depth_first_traversal_callback_t` for an explanation of this method. Note
    that we don't return a `continue_bool_t` here; that's because these are called out of
    sync with respect to `handle_pair()`, so allowing `filter_range()` to abort the
    traversal would be confusing. The other `depth_first_traversal_callback_t` methods
    could be passed through, but we don't simply because there's no immediate use for
    them. */
    virtual void filter_range(
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl,
            bool *skip_out) {
        *skip_out = false;
    }

    // Passes a keyvalue and a callback.  waiter.wait_interruptible() must be called to
    // begin the region of "exclusive access", which only handle_pair implementation
    // can enters at a time.  (This should happen after loading the value from disk
    // (which should be done concurrently) and before using ql::env_t to evaluate
    // transforms and terminals, or whatever non-reentrant behavior you have in mind.)
    virtual continue_bool_t handle_pair(
            scoped_key_value_t &&keyvalue,
            concurrent_traversal_fifo_enforcer_signal_t waiter)
            THROWS_ONLY(interrupted_exc_t) = 0;

    virtual profile::trace_t *get_trace() THROWS_NOTHING { return nullptr; }

protected:
    virtual ~concurrent_traversal_callback_t() { }
private:
    DISABLE_COPYING(concurrent_traversal_callback_t);
};

continue_bool_t btree_concurrent_traversal(
        superblock_t *superblock,
        const key_range_t &range,
        concurrent_traversal_callback_t *cb,
        direction_t direction,
        release_superblock_t release_superblock);

#endif  // BTREE_CONCURRENT_TRAVERSAL_HPP_
