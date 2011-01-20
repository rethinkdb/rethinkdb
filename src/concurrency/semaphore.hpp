
#ifndef __CONCURRENCY_SEMAPHORE_HPP__
#define __CONCURRENCY_SEMAPHORE_HPP__

#include "concurrency/rwi_lock.hpp"   // For lock_available_callback_t
#include "concurrency/cond_var.hpp"

struct semaphore_available_callback_t {
    virtual void on_semaphore_available() = 0;
};

class semaphore_t {
    struct lock_request_t :
        public intrusive_list_node_t<lock_request_t>,
        public thread_message_t
    {
        semaphore_available_callback_t *cb;
        void on_thread_switch() {
            cb->on_semaphore_available();
            delete this;
        }
    };

    int capacity, current;
    intrusive_list_t<lock_request_t> waiters;

public:
    semaphore_t(int cap) : capacity(cap), current(0) { }

    void lock(semaphore_available_callback_t *cb) {
        if (current == capacity) {
            lock_request_t *r = new lock_request_t;
            r->cb = cb;
            waiters.push_back(r);
        } else {
            current++;
            cb->on_semaphore_available();
        }
    }

    void co_lock() {
        struct : public semaphore_available_callback_t, public cond_t {
            void on_semaphore_available() { pulse(); }
        } cb;
        lock(&cb);
        cb.wait();
    }

    void unlock() {
        rassert(current > 0);
        if (lock_request_t *h = waiters.head()) {
            waiters.remove(h);
            call_later_on_this_thread(h);
        } else {
            current--;
        }
    }

    void lock_now() {
        rassert(current < capacity);
        current++;
    }
};

#endif /* __CONCURRENCY_SEMAPHORE_HPP__ */
