
#ifndef __CONCURRENCY_MUTEX_HPP__
#define __CONCURRENCY_MUTEX_HPP__

#include "concurrency/rwi_lock.hpp"   // For lock_available_callback_t

class mutex_t {
    struct lock_request_t :
        public intrusive_list_node_t<lock_request_t>,
        public cpu_message_t
    {
        lock_available_callback_t *cb;
        void on_cpu_switch() {
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

    void unlock() {
        assert(locked);
        if (lock_request_t *h = waiters.head()) {
            waiters.remove(h);
            call_later_on_this_cpu(h);
        } else {
            locked = false;
        }
    }

    void lock_now() {
        assert(!locked);
        locked = true;
    }
};

void co_lock_mutex(mutex_t *mutex);

struct mutex_acquisition_t {
    mutex_t *lock;
    mutex_acquisition_t(mutex_t *lock) : lock(lock) {
        co_lock_mutex(lock);
    }
    ~mutex_acquisition_t() {
        lock->unlock();
    }
};


#endif /* __CONCURRENCY_MUTEX_HPP__ */

