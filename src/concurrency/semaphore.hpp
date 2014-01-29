// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_SEMAPHORE_HPP_
#define CONCURRENCY_SEMAPHORE_HPP_

#include "concurrency/signal.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/intrusive_list.hpp"

class semaphore_available_callback_t {
public:
    virtual ~semaphore_available_callback_t() {}
    virtual void on_semaphore_available() = 0;
};

#define SEMAPHORE_NO_LIMIT (-1)

class semaphore_t {
public:
    virtual ~semaphore_t() { }

    virtual void lock(semaphore_available_callback_t *cb, int count = 1) = 0;
    virtual void co_lock(int count = 1) = 0;
    virtual void co_lock_interruptible(signal_t *interruptor, int count = 1) = 0;
    virtual void unlock(int count = 1) = 0;
    virtual void lock_now(int count = 1) = 0;
    virtual void force_lock(int count = 1) = 0;
};


/* `static_semaphore_t` is a `semaphore_t` with a fixed capacity that has
 to be provided at the time of construction. */

class static_semaphore_t : public semaphore_t {
    struct lock_request_t : public intrusive_list_node_t<lock_request_t> {
        semaphore_available_callback_t *cb;
        int count;
        void on_available() {
            cb->on_semaphore_available();
            delete this;
        }
    };

    int capacity, current;
    intrusive_list_t<lock_request_t> waiters;

#ifndef NDEBUG
    bool in_callback;
#endif

public:
    explicit static_semaphore_t(int cap) : capacity(cap), current(0)
#ifndef NDEBUG
                         , in_callback(false)
#endif
    {
        rassert(capacity >= 0 || capacity == SEMAPHORE_NO_LIMIT);
    }
    ~static_semaphore_t() {
        rassert(waiters.empty());
    }

    void lock(semaphore_available_callback_t *cb, int count = 1);

    void co_lock(int count = 1);

    void co_lock_interruptible(signal_t *interruptor, int count = 1);

    void unlock(int count = 1);
    void lock_now(int count = 1);
    void force_lock(int count = 1);
};

/* `adjustable_semaphore_t` is a `semaphore_t` where you can change the
capacity at runtime. If you call `set_capacity()` and the new capacity is less
than the current number of objects that hold the semaphore, then new objects
will be allowed to enter at `trickle_fraction` of the rate that the objects are
leaving until the number of objects drops to the desired capacity. */

class adjustable_semaphore_t : public semaphore_t {
    struct lock_request_t : public intrusive_list_node_t<lock_request_t> {
        semaphore_available_callback_t *cb;
        int count;
        void on_available() {
            cb->on_semaphore_available();
            delete this;
        }
    };

    int capacity, current;
    double trickle_fraction, trickle_points;
    intrusive_list_t<lock_request_t> waiters;

#ifndef NDEBUG
    bool in_callback;
#endif

public:
    explicit adjustable_semaphore_t(int cap, double tf = 0.0) :
        capacity(cap), current(0), trickle_fraction(tf), trickle_points(0)
#ifndef NDEBUG
        , in_callback(false)
#endif
    {
        rassert(trickle_fraction <= 1.0 && trickle_fraction >= 0.0);
        rassert(capacity >= 0 || capacity == SEMAPHORE_NO_LIMIT);
    }
    ~adjustable_semaphore_t() {
        rassert(waiters.empty());
    }

    void lock(semaphore_available_callback_t *cb, int count = 1);

    void co_lock(int count = 1);
    void co_lock_interruptible(signal_t *interruptor, int count = 1);

    void unlock(int count = 1);
    void lock_now(int count = 1);
    void force_lock(int count = 1);

    void set_capacity(int new_capacity);

    int get_capacity() const { return capacity; }

private:
    bool try_lock(int count);

    void pump();
};


class semaphore_acq_t {
public:
    semaphore_acq_t(semaphore_t *_acquiree, int _count = 1, bool _force = false) :
        acquiree(_acquiree),
        count(_count) {

        if (_force) {
            acquiree->force_lock(count);
        } else {
            acquiree->co_lock(count);
        }
    }

    semaphore_acq_t(semaphore_acq_t &&movee) :
        acquiree(movee.acquiree),
        count(movee.count) {

        movee.acquiree = NULL;
    }

    ~semaphore_acq_t() {
        if (acquiree) {
            acquiree->unlock(count);
        }
    }

private:
    semaphore_t *acquiree;
    int count;
    DISABLE_COPYING(semaphore_acq_t);
};

#endif /* CONCURRENCY_SEMAPHORE_HPP_ */
