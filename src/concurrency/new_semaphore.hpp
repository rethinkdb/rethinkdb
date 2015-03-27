// Copyright 2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_NEW_SEMAPHORE_HPP_
#define CONCURRENCY_NEW_SEMAPHORE_HPP_

#include "concurrency/cond_var.hpp"
#include "containers/intrusive_list.hpp"

// This semaphore obeys first-in-line/first-acquisition semantics.  The
// new_semaphore_acq_t's will receive access to the semaphore in the same order that
// such access was requested.  Also, there aren't problems with starvation.  Also, it
// doesn't have naked lock and unlock functions, you have to use new_semaphore_acq_t.

class new_semaphore_acq_t;

class new_semaphore_t : public home_thread_mixin_debug_only_t {
public:
    explicit new_semaphore_t(int64_t capacity);
    ~new_semaphore_t();

    int64_t capacity() const { return capacity_; }
    int64_t current() const { return current_; }

    void set_capacity(int64_t new_capacity);

private:
    friend class new_semaphore_acq_t;
    void add_acquirer(new_semaphore_acq_t *acq);
    void remove_acquirer(new_semaphore_acq_t *acq);

    void pulse_waiters();

    // Normally, current_ <= capacity_, and capacity_ doesn't change.  current_ can
    // exceed capacity_ for three reasons.
    //   1. A call to change_acquired_count could force it to overflow.
    //   2. An acquirer will never be blocked while current_ is 0.
    //   3. An act of God could change capacity_ (currently unimplemented; hail Satan).
    int64_t capacity_;
    int64_t current_;

    intrusive_list_t<new_semaphore_acq_t> waiters_;

    DISABLE_COPYING(new_semaphore_t);
};

class new_semaphore_acq_t : public intrusive_list_node_t<new_semaphore_acq_t> {
public:
    // Construction is non-blocking, it gets you in line for the semaphore.  You need
    // to call acquisition_signal()->wait() in order to wait for your acquisition of the
    // semaphore.  Acquirers receive the semaphore in the same order that they've
    // acquired it.
    ~new_semaphore_acq_t();
    new_semaphore_acq_t();
    new_semaphore_acq_t(new_semaphore_t *semaphore, int64_t count);
    new_semaphore_acq_t(new_semaphore_acq_t &&movee);
    new_semaphore_acq_t &operator=(new_semaphore_acq_t &&movee);

    // Returns "how much" of the semaphore this acq has acquired or would acquire.
    int64_t count() const;

    // Changes "how much" of the semaphore this acq has acquired or would acquire.
    // If it's already acquired the semaphore, and new_count is bigger than the
    // current value of acquired_count(), it's possible that you'll make the
    // semaphore "overfull", meaning that current_ > capacity_.  That's not ideal,
    // but it's O.K.
    void change_count(int64_t new_count);

    // Transfers the "semaphore credits" that `other` owns to `this` and resets `other`.
    // `other` must have already acquired the semaphore; `this` must have already
    // acquired the semaphore or be uninitialized.
    //
    // For example, if `this->count() == 1` and `other->count() == 2` before the call,
    // then after the call `this->count() == 3` and `other` is invalid.
    void transfer_in(new_semaphore_acq_t &&other);

    // Initializes the object.
    void init(new_semaphore_t *semaphore, int64_t count);

    void reset();

    // Returns a signal that gets pulsed when this has successfully acquired the
    // semaphore.
    const signal_t *acquisition_signal() const { return &cond_; }

private:
    friend class new_semaphore_t;

    // The semaphore this acquires (or NULL if this hasn't acquired a semaphore yet).
    new_semaphore_t *semaphore_;

    // The count of "how much" of the semaphore we've acquired (if semaphore_ is
    // non-NULL).
    int64_t count_;

    // Gets pulsed when we have successfully acquired the semaphore.
    cond_t cond_;
    DISABLE_COPYING(new_semaphore_acq_t);
};



#endif  // CONCURRENCY_NEW_SEMAPHORE_HPP_
