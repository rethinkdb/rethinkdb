#ifndef CONCURRENCY_NEW_MUTEX_HPP_
#define CONCURRENCY_NEW_MUTEX_HPP_

#include "concurrency/interruptor.hpp"
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

    void guarantee_is_for_lock(const new_mutex_t *mutex) const {
        rwlock_in_line_.guarantee_is_for_lock(&mutex->rwlock_);
    }

private:
    rwlock_in_line_t rwlock_in_line_;

    DISABLE_COPYING(new_mutex_in_line_t);
};

class new_mutex_acq_t {
public:
    // Acquires the lock.  The constructor blocks the coroutine, it doesn't return
    // until the lock is acquired.
    explicit new_mutex_acq_t(new_mutex_t *lock) : in_line(lock) {
        in_line.acq_signal()->wait();
    }

    // Acquires the lock.  The constructor blocks the coroutine until the lock
    // is acquired or the interruptor is pulsed.
    new_mutex_acq_t(new_mutex_t *lock, signal_t *interruptor) : in_line(lock) {
        wait_interruptible(in_line.acq_signal(), interruptor);

    }

    void guarantee_is_holding(const new_mutex_t *new_mutex) const {
        in_line.guarantee_is_for_lock(new_mutex);
    }

private:
    new_mutex_in_line_t in_line;
    DISABLE_COPYING(new_mutex_acq_t);
};

#endif  // CONCURRENCY_NEW_MUTEX_HPP_
