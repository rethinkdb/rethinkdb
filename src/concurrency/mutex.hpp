// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_MUTEX_HPP_
#define CONCURRENCY_MUTEX_HPP_

#include <deque>
#include <utility>

#include "errors.hpp"

class coro_t;
class mutex_t;

void co_lock_mutex(mutex_t *mutex);
void unlock_mutex(mutex_t *mutex, bool eager = false);

// Deprecated.  You should use new_mutex_t in new_mutex.hpp instead.
class mutex_t {
public:
    class acq_t {
    public:
        acq_t() : lock_(nullptr), eager_(false) { }
        explicit acq_t(acq_t &&other) : lock_(nullptr), eager_(false) {
            swap(*this, other);
        }
        explicit acq_t(mutex_t *l, bool eager = false);
        ~acq_t();
        void reset();
        void reset(mutex_t *l, bool eager = false);
        void assert_is_holding(DEBUG_VAR mutex_t *m) const {
            rassert(lock_ == m);
        }
    private:
        friend void swap(acq_t &, acq_t &);
        mutex_t *lock_;
        bool eager_;
        DISABLE_COPYING(acq_t);
    };

    mutex_t() : locked(false) { }
    ~mutex_t() { rassert(!locked); }

    bool is_locked() {
        return locked;
    }

    friend void co_lock_mutex(mutex_t *mutex);
    friend void unlock_mutex(mutex_t *mutex, bool eager);

private:
    bool locked;
    std::deque<coro_t *> waiters;

    DISABLE_COPYING(mutex_t);
};

inline void swap(mutex_t::acq_t &a, mutex_t::acq_t &b) {
    std::swap(a.lock_, b.lock_);
    std::swap(a.eager_, b.eager_);
}

#endif /* CONCURRENCY_MUTEX_HPP_ */

