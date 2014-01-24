// Copyright 2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_NEW_SEMAPHORE_HPP_
#define CONCURRENCY_NEW_SEMAPHORE_HPP_

#include "concurrency/cond_var.hpp"
#include "containers/intrusive_list.hpp"

// RSI: This is a horrible name, fix it.

class new_semaphore_acq_t;

class new_semaphore_t {
public:
    explicit new_semaphore_t(int64_t capacity);
    ~new_semaphore_t();

private:
    friend class new_semaphore_acq_t;
    void add_acquirer(new_semaphore_acq_t *acq);
    void remove_acquirer(new_semaphore_acq_t *acq);

    void pulse_waiters();

    int64_t capacity_;
    int64_t current_;

    intrusive_list_t<new_semaphore_acq_t> waiters_;

    DISABLE_COPYING(new_semaphore_t);
};

class new_semaphore_acq_t : public intrusive_list_node_t<new_semaphore_acq_t> {
public:
    // Construction is non-blocking, it gets you in line for the semaphore.  You need
    // to call acquisition()->wait() in order to wait for your acquisition of the
    // semaphore.  Acquirers receive the semaphore in the same order that they've
    // acquired it.
    ~new_semaphore_acq_t();
    new_semaphore_acq_t();
    new_semaphore_acq_t(new_semaphore_t *semaphore, int64_t count);

    void init(new_semaphore_t *semaphore, int64_t count);

    // Returns a signal that gets pulsed when this has successfully acquired the
    // semaphore.
    const signal_t *acquisition() const { return &cond_; }

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
