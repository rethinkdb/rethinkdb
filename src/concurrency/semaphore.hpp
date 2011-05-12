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
        int count;
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

    void lock(semaphore_available_callback_t *cb, int count = 1) {
        rassert(count <= capacity || capacity == SEMAPHORE_NO_LIMIT);
        if (current + count <= capacity || capacity == SEMAPHORE_NO_LIMIT) {
            current += count;
            cb->on_semaphore_available();
        } else {
            lock_request_t *r = new lock_request_t;
            r->count = count;
            r->cb = cb;
            waiters.push_back(r);
        }
    }

    void co_lock(int count = 1) {
        struct : public semaphore_available_callback_t, public cond_t {
            void on_semaphore_available() { pulse(); }
        } cb;
        lock(&cb, count);
        cb.wait();
    }

    void unlock(int count = 1) {
        rassert(current >= count);
        current -= count;
        if (lock_request_t *h = waiters.head()) {
            if (current + h->count <= capacity) {
                waiters.remove(h);
                current += h->count;
                call_later_on_this_thread(h);
            }
        }
    }

    void lock_now(int count = 1) {
        rassert(current + count <= capacity || capacity == SEMAPHORE_NO_LIMIT);
        current += count;
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
        int count;
        void on_thread_switch() {
            cb->on_semaphore_available();
            delete this;
        }
    };

    int capacity, current;
    float trickle_fraction, trickle_points;
    intrusive_list_t<lock_request_t> waiters;

public:
    adjustable_semaphore_t(int cap, float tf = 0.0) :
        capacity(cap), current(0), trickle_fraction(tf), trickle_points(0)
    {
        rassert(trickle_fraction <= 1.0 && trickle_fraction >= 0.0);
        rassert(capacity >= 0 || capacity == SEMAPHORE_NO_LIMIT);
    }

    void lock(semaphore_available_callback_t *cb, int count = 1) {
        lock_request_t *r = new lock_request_t;
        r->cb = cb;
        r->count = count;
        waiters.push_back(r);
        pump();
    }

    void co_lock(int count = 1) {
        struct : public semaphore_available_callback_t, public cond_t {
            void on_semaphore_available() { pulse(); }
        } cb;
        lock(&cb, count);
        cb.wait();
    }

    void unlock(int count = 1) {
        rassert(current > 0);
        current -= count;
        if (current > capacity && capacity != SEMAPHORE_NO_LIMIT) {
            trickle_points += trickle_fraction * count;
        }
        pump();
    }

    void lock_now(int count = 1) {
        if (!try_lock(count)) {
            crash("lock_now() can't lock now.\n");
        }
    }

    void set_capacity(int new_capacity) {
        capacity = new_capacity;
        rassert(capacity >= 0 || capacity == SEMAPHORE_NO_LIMIT);
        pump();
    }

private:
    bool try_lock(int count) {
        if (current + count > capacity && capacity != SEMAPHORE_NO_LIMIT) {
            if (trickle_points >= count) {
                trickle_points -= count;
            } else {
                return false;
            }
        }
        current += count;
        return true;
    }

    void pump() {
        lock_request_t *h;
        while ((h = waiters.head()) && try_lock(h->count)) {
            waiters.remove(h);
            call_later_on_this_thread(h);
        }
    }
};

#endif /* __CONCURRENCY_SEMAPHORE_HPP__ */
