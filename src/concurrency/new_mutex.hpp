#ifndef CONCURRENCY_NEW_MUTEX_HPP_
#define CONCURRENCY_NEW_MUTEX_HPP_

#include "concurrency/rwlock.hpp"

// This is a mutex, following the usual semantics (seen in rwlock_t,
// fifo_enforcer_sink_t, buf_lock_t) where construction puts you in the queue, a
// signal is pulsed when you have exclusive access, and acquirers gain exclusive
// access in the same order as construction.  You should favor this instead of
// mutex_t.

class new_mutex_t {
public:
    new_mutex_t() { }
    ~new_mutex_t() { }

private:
    friend class new_mutex_in_line_t;

    rwlock_t rwlock_;

    DISABLE_COPYING(new_mutex_t);
};

class new_mutex_in_line_t {
public:
    new_mutex_in_line_t() { }

    explicit new_mutex_in_line_t(new_mutex_t *new_mutex)
        : rwlock_in_line_(&new_mutex->rwlock_, access_t::write) { }

    const signal_t *acq_signal() const {
        return rwlock_in_line_.write_signal();
    }

    void reset() {
        rwlock_in_line_.reset();
    }

    void assert_is_for_lock(new_mutex_t *mutex) {
        rwlock_in_line_.assert_is_for_lock(&mutex->rwlock_);
    }

private:
    rwlock_in_line_t rwlock_in_line_;

    DISABLE_COPYING(new_mutex_in_line_t);
};

class new_mutex_acq_t : private new_mutex_in_line_t {
public:
    new_mutex_acq_t(new_mutex_t *new_mutex) :
            new_mutex_in_line_t(new_mutex) {
        acq_signal()->wait();
    }

    void assert_is_holding(new_mutex_t *new_mutex) {
        new_mutex_in_line_t::assert_is_for_lock(new_mutex);
    }

private:
    DISABLE_COPYING(new_mutex_acq_t);
};

#endif  // CONCURRENCY_NEW_MUTEX_HPP_
