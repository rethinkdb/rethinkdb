// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "concurrency/semaphore.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/interruptor.hpp"

void static_semaphore_t::lock(semaphore_available_callback_t *cb, int64_t count) {
    rassert(!in_callback);
    rassert(count <= capacity || capacity == SEMAPHORE_NO_LIMIT);
    if (current + count <= capacity || capacity == SEMAPHORE_NO_LIMIT) {
        current += count;
        DEBUG_ONLY_CODE(in_callback = true);
        cb->on_semaphore_available();
        rassert(in_callback);
        DEBUG_ONLY_CODE(in_callback = false);
    } else {
        lock_request_t *r = new lock_request_t;
        r->count = count;
        r->cb = cb;
        waiters.push_back(r);
    }
}

void static_semaphore_t::co_lock(int64_t count) {
    rassert(!in_callback);
    struct : public semaphore_available_callback_t, public one_waiter_cond_t {
        void on_semaphore_available() { pulse(); }
    } cb;
    lock(&cb, count);
    cb.wait_ordered();
    // TODO: Remove the need for in_callback checks.
}

void static_semaphore_t::co_lock_interruptible(signal_t *interruptor, int64_t count) {
    rassert(!in_callback);
    struct : public semaphore_available_callback_t, public cond_t {
        void on_semaphore_available() { pulse(); }
    } cb;
    lock(&cb, count);

    try {
        wait_interruptible(&cb, interruptor);
    } catch (const interrupted_exc_t &ex) {
        // Remove our lock request from the queue
        for (lock_request_t *request = waiters.head(); request != nullptr; request = waiters.next(request)) {
            if (request->cb == &cb) {
                waiters.remove(request);
                delete request;
                break;
            }
        }
        throw;
    }
}

void static_semaphore_t::unlock(int64_t count) {
    rassert(!in_callback);
    rassert(current >= count);
    current -= count;
    while (lock_request_t *h = waiters.head()) {
        if (current + h->count <= capacity) {
            waiters.remove(h);
            current += h->count;
            rassert(!in_callback);
            DEBUG_ONLY_CODE(in_callback = true);
            h->on_available();
            rassert(in_callback);
            DEBUG_ONLY_CODE(in_callback = false);
        } else {
            break;
        }
    }
}

void static_semaphore_t::lock_now(int64_t count) {
    rassert(!in_callback);
    rassert(current + count <= capacity || capacity == SEMAPHORE_NO_LIMIT);
    current += count;
}

void static_semaphore_t::force_lock(int64_t count) {
    current += count;
}

void adjustable_semaphore_t::lock(semaphore_available_callback_t *cb, int64_t count) {
    rassert(!in_callback);
    rassert(count <= capacity || capacity == SEMAPHORE_NO_LIMIT);
    if (try_lock(count)) {
        rassert(!in_callback);
        DEBUG_ONLY_CODE(in_callback = true);
        cb->on_semaphore_available();
        rassert(in_callback);
        DEBUG_ONLY_CODE(in_callback = false);
    } else {
        lock_request_t *r = new lock_request_t;
        r->count = count;
        r->cb = cb;
        waiters.push_back(r);
    }
}

void adjustable_semaphore_t::co_lock(int64_t count) {
    rassert(!in_callback);
    struct : public semaphore_available_callback_t, public one_waiter_cond_t {
        void on_semaphore_available() { pulse(); }
    } cb;
    lock(&cb, count);
    cb.wait_ordered();
    // TODO: remove need for in_callback checks
}

void adjustable_semaphore_t::co_lock_interruptible(signal_t *interruptor, int64_t count) {
    rassert(!in_callback);
    struct : public semaphore_available_callback_t, public cond_t {
        void on_semaphore_available() { pulse(); }
    } cb;
    lock(&cb, count);

    try {
        wait_interruptible(&cb, interruptor);
    } catch (const interrupted_exc_t& ex) {
        // Remove our lock request from the queue
        for (lock_request_t *request = waiters.head(); request != nullptr; request = waiters.next(request)) {
            if (request->cb == &cb) {
                waiters.remove(request);
                delete request;
                break;
            }
        }
        throw;
    }
}

void adjustable_semaphore_t::unlock(int64_t count) {
    rassert(!in_callback);
    rassert(current >= count);
    current -= count;
    if (current > capacity && capacity != SEMAPHORE_NO_LIMIT) {
        trickle_points += trickle_fraction * count;
    }
    pump();
}

void adjustable_semaphore_t::lock_now(int64_t count) {
    rassert(!in_callback);
    if (!try_lock(count)) {
        crash("lock_now() can't lock now.\n");
    }
}

void adjustable_semaphore_t::force_lock(int64_t count) {
    current += count;
}

void adjustable_semaphore_t::set_capacity(int64_t new_capacity) {
    rassert(!in_callback);
    capacity = new_capacity;
    rassert(capacity >= 0 || capacity == SEMAPHORE_NO_LIMIT);
    pump();
}

bool adjustable_semaphore_t::try_lock(int64_t count) {
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

void adjustable_semaphore_t::pump() {
    rassert(!in_callback);
    lock_request_t *h;
    while ((h = waiters.head()) && try_lock(h->count)) {
        waiters.remove(h);
        rassert(!in_callback);
        DEBUG_ONLY_CODE(in_callback = true);
        h->on_available();
        rassert(in_callback);
        DEBUG_ONLY_CODE(in_callback = false);
    }
}
