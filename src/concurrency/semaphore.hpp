#ifndef __CONCURRENCY_SEMAPHORE_HPP__
#define __CONCURRENCY_SEMAPHORE_HPP__

#include "concurrency/rwi_lock.hpp"   // For lock_available_callback_t
#include "concurrency/cond_var.hpp"

struct semaphore_available_callback_t {
    virtual ~semaphore_available_callback_t() {}
    virtual void on_semaphore_available() = 0;
};

#define SEMAPHORE_NO_LIMIT (-1)

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
    semaphore_t(int cap) : capacity(cap), current(0) {
        rassert(capacity >= 0 || capacity == SEMAPHORE_NO_LIMIT);
    }

    void lock(semaphore_available_callback_t *cb) {
        if (current < capacity || capacity == SEMAPHORE_NO_LIMIT) {
            current++;
            cb->on_semaphore_available();
        } else {
            lock_request_t *r = new lock_request_t;
            r->cb = cb;
            waiters.push_back(r);
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
        rassert(current < capacity || capacity == SEMAPHORE_NO_LIMIT);
        current++;
    }
};

/* `adjustable_semaphore_t` is a `semaphore_t` where you can change the
capacity at runtime. If you call `set_capacity()` and the new capacity is less
than the current number of objects that hold the semaphore, then new objects
will be allowed to enter at `trickle_fraction` of the rate that the objects are
leaving until the number of objects drops to the desired capacity. */

class adjustable_semaphore_t {
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
    float trickle_fraction, trickle_point;
    intrusive_list_t<lock_request_t> waiters;

public:
    adjustable_semaphore_t(int cap, float tf = 0.0) :
        capacity(cap), current(0), trickle_fraction(tf), trickle_point(0)
    {
        rassert(trickle_fraction <= 1.0 && trickle_fraction >= 0.0);
        rassert(capacity >= 0 || capacity == SEMAPHORE_NO_LIMIT);
    }

    void lock(semaphore_available_callback_t *cb) {
        lock_request_t *r = new lock_request_t;
        r->cb = cb;
        waiters.push_back(r);
        pump();
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
        current--;
        trickle_point += trickle_fraction;
        if ((current > capacity && capacity != SEMAPHORE_NO_LIMIT) &&
                int(trickle_point) != int(trickle_point + trickle_fraction)) {
            /* So as to avoid completely locking out clients when the capacity
            is adjusted abruptly, let some fraction of the clients "cheat" and
            bypass the capacity check. For every `N` times `unlock()` is called,
            we will take this branch in `N * trickle_fraction` of them. */ 
            pump(true);
        } else {
            pump();
        }
    }

    void lock_now() {
        rassert(current < capacity);
        current++;
    }

    void set_capacity(int new_capacity) {
        rassert(capacity >= 0 || capacity == SEMAPHORE_NO_LIMIT);
        capacity = new_capacity;
        pump();
    }

private:
    void pump(bool cheat = false) {
        lock_request_t *h;
        while ((h = waiters.head()) &&
                (cheat || current < capacity || capacity == SEMAPHORE_NO_LIMIT)) {
            waiters.remove(h);
            call_later_on_this_thread(h);
            current++;
        }
    }
};

#endif /* __CONCURRENCY_SEMAPHORE_HPP__ */
