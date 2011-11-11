#ifndef __CONCURRENCY_MUTEX_HPP__
#define __CONCURRENCY_MUTEX_HPP__

#include "concurrency/rwi_lock.hpp"   // For lock_available_callback_t
#include "arch/runtime/runtime_utils.hpp"

class mutex_t {
    struct lock_request_t :
        public intrusive_list_node_t<lock_request_t>,
        public thread_message_t
    {
        lock_available_callback_t *cb;
        void on_thread_switch() {
            cb->on_lock_available();
            delete this;
        }
    };

    bool locked;
    intrusive_list_t<lock_request_t> waiters;

public:
    mutex_t() : locked(false) { }

    void lock(lock_available_callback_t *cb) {
        if (locked) {
            lock_request_t *r = new lock_request_t;
            r->cb = cb;
            waiters.push_back(r);
        } else {
            locked = true;
            cb->on_lock_available();
        }
    }

    void unlock(bool eager = false);

    void lock_now() {
        rassert(!locked);
        locked = true;
    }

    bool is_locked() {
        return locked;
    }
};

void co_lock_mutex(mutex_t *mutex);

class mutex_acquisition_t {
public:
    mutex_acquisition_t(mutex_t *lock, bool eager = false) : lock_(lock), eager_(eager) {
        co_lock_mutex(lock_);
    }
    mutex_acquisition_t() : lock_(NULL) { }
    ~mutex_acquisition_t() {
        if (lock_) lock_->unlock(eager_);
    }
    void assert_is_holding(UNUSED mutex_t *m) const {
        rassert(lock_ == m);
    }
private:
    friend void swap(mutex_acquisition_t &a, mutex_acquisition_t &b);
    mutex_t *lock_;
    bool eager_;
    DISABLE_COPYING(mutex_acquisition_t);
};

inline void swap(mutex_acquisition_t &a, mutex_acquisition_t &b) {
    std::swap(a.lock_, b.lock_);
    std::swap(a.eager_, b.eager_);
}

#endif /* __CONCURRENCY_MUTEX_HPP__ */

