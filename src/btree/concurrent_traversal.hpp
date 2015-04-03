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

    /* See `depth_first_traversal_callback_t` for an explanation of these methods */
    virtual continue_bool_t filter_range(
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl,
            bool *skip_out) {
        *skip_out = false;
        return continue_bool_t::CONTINUE;
    }
    virtual continue_bool_t filter_range_ts(
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl,
            UNUSED repli_timestamp_t timestamp,
            bool *skip_out) {
        *skip_out = false;
        return continue_bool_t::CONTINUE;
    }
    virtual continue_bool_t handle_pre_leaf(
            UNUSED const counted_t<counted_buf_lock_t> &buf_lock,
            UNUSED const counted_t<counted_buf_read_t> &buf_read,
            UNUSED const btree_key_t *left_excl_or_null,
            UNUSED const btree_key_t *right_incl,
            bool *skip_out) {
        *skip_out = false;
        return continue_bool_t::CONTINUE;
    }

    /* Called before every call to `handle_pair`. If it sets `*skip_out` to `false`, no
    call to `handle_pair()` will be generated. The reason we provide this method here and
    not in `depth_first_traversal_callback_t` is because here because we spawn a
    coroutine for every call to `handle_pair()`, and if we're only interested in a
    fraction of key-value pairs, this will reduce the number of coroutines we spawn. */
    virtual continue_bool_t filter_key(
            UNUSED const btree_key_t *key,
            bool *skip_out) {
        *skip_out = false;
        return continue_bool_t::CONTINUE;
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

    virtual profile::trace_t *get_trace() THROWS_NOTHING { return NULL; }

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
