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

enum class done_t { NO, YES };
class concurrent_traversal_callback_t {
public:
    concurrent_traversal_callback_t() { }

    // Passes a keyvalue and a callback.  waiter.wait_interruptible() must be called to
    // begin the region of "exclusive access", which only handle_pair implementation
    // can enters at a time.  (This should happen after loading the value from disk
    // (which should be done concurrently) and before using ql::env_t to evaluate
    // transforms and terminals, or whatever non-reentrant behavior you have in mind.)
    virtual done_t handle_pair(scoped_key_value_t &&keyvalue,
                               concurrent_traversal_fifo_enforcer_signal_t waiter)
        THROWS_ONLY(interrupted_exc_t) = 0;

    virtual profile::trace_t *get_trace() THROWS_NOTHING { return NULL; }

protected:
    virtual ~concurrent_traversal_callback_t() { }
private:
    DISABLE_COPYING(concurrent_traversal_callback_t);
};

bool btree_concurrent_traversal(superblock_t *superblock, const key_range_t &range,
                                concurrent_traversal_callback_t *cb,
                                direction_t direction);



#endif  // BTREE_CONCURRENT_TRAVERSAL_HPP_
