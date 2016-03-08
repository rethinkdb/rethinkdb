// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_CROSS_THREAD_MUTEX_HPP_
#define CONCURRENCY_CROSS_THREAD_MUTEX_HPP_

#include <deque>
#include <utility>

#include "arch/spinlock.hpp"

class coro_t;
class cross_thread_mutex_t;

/* `cross_thread_mutex_t` behaves like `mutex_t`, but can be used across
 * multiple threads.
 * It also allows recursive lock acquisition from the same coroutine if desired.
 * It is more efficient than doing
 * `{ on_thread_t t(mutex_home_thread); co_lock_mutex(&m); }`
 * because the thread switch is avoided.
 * In single-threaded use cases, it comes with more overhead than a regular
 * `mutex_t` though. */
class cross_thread_mutex_t {
public:
    class acq_t {
    public:
        acq_t() : lock_(nullptr) { }
        explicit acq_t(cross_thread_mutex_t *l);
        ~acq_t();
        void reset();
        void reset(cross_thread_mutex_t *l);
        void assert_is_holding(DEBUG_VAR cross_thread_mutex_t *m) const {
            rassert(lock_ == m);
        }
        void swap(cross_thread_mutex_t::acq_t &other) {
            std::swap(lock_, other.lock_);
        }
    private:
        friend void swap(acq_t &, acq_t &);
        cross_thread_mutex_t *lock_;
        DISABLE_COPYING(acq_t);
    };

    explicit cross_thread_mutex_t(bool recursive = false) :
        is_recursive(recursive), locked_count(0), lock_holder(nullptr) { }
    ~cross_thread_mutex_t() {
#ifndef NDEBUG
        spinlock_acq_t acq(&spinlock);
        rassert(locked_count == 0);
#endif
    }

    bool is_locked() {
        spinlock_acq_t acq(&spinlock);
        return locked_count > 0;
    }

private:
    // These are private. Use `acq_t` to lock and unlock the mutex.
    void co_lock();
    void unlock();

    spinlock_t spinlock;
    bool is_recursive;
    int locked_count;
    coro_t *lock_holder;
    std::deque<coro_t *> waiters;

    DISABLE_COPYING(cross_thread_mutex_t);
};

#endif /* CONCURRENCY_CROSS_THREAD_MUTEX_HPP_ */
