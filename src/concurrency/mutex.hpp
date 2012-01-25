#ifndef __CONCURRENCY_MUTEX_HPP__
#define __CONCURRENCY_MUTEX_HPP__

#include <deque>

#include "utils.hpp"

class coro_t;

class mutex_t {
public:
    class acq_t {
    public:
        acq_t() : lock_(NULL), eager_(false) { }
        explicit acq_t(mutex_t *l, bool eager = false);
        ~acq_t();
        void reset();
        void reset(mutex_t *l, bool eager = false);
        void assert_is_holding(UNUSED mutex_t *m) const {
            rassert(lock_ == m);
        }
    private:
        friend void swap(acq_t &, acq_t &);
        mutex_t *lock_;
        bool eager_;
        DISABLE_COPYING(acq_t);
    };

    mutex_t() : locked(false) { }

    bool is_locked() {
        return locked;
    }

private:
    bool locked;
    std::deque<coro_t *> waiters;
};

inline void swap(mutex_t::acq_t &a, mutex_t::acq_t &b) {
    std::swap(a.lock_, b.lock_);
    std::swap(a.eager_, b.eager_);
}

#endif /* __CONCURRENCY_MUTEX_HPP__ */

