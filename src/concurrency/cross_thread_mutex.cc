// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "concurrency/cross_thread_mutex.hpp"

#include "arch/runtime/coroutines.hpp"


cross_thread_mutex_t::acq_t::acq_t(cross_thread_mutex_t *l) : lock_(nullptr) {
    reset(l);
}

cross_thread_mutex_t::acq_t::~acq_t() {
    reset();
}

void cross_thread_mutex_t::acq_t::reset() {
    if (lock_) {
        lock_->unlock();
    }
    lock_ = nullptr;
}

void cross_thread_mutex_t::acq_t::reset(cross_thread_mutex_t *l) {
    reset();
    lock_ = l;
    lock_->co_lock();
}

void cross_thread_mutex_t::co_lock() {
    bool do_wait = false;
    {
        spinlock_acq_t acq(&spinlock);
        if (locked_count > 0 && lock_holder != coro_t::self()) {
            waiters.push_back(coro_t::self());
            do_wait = true;
        } else {
            guarantee(locked_count == 0 || is_recursive,
                "Deadlock detected. A coroutine tried to acquire a mutex which it is "
                "already holding.");
            lock_holder = coro_t::self();
            ++locked_count;
        }
    }
    if (do_wait) {
        coro_t::wait();
    }
}

void cross_thread_mutex_t::unlock() {
    spinlock_acq_t acq(&spinlock);
    rassert(locked_count > 0);
    rassert(lock_holder == coro_t::self());
    --locked_count;
    if (locked_count == 0 && !waiters.empty()) {
        coro_t *next = waiters.front();
        waiters.pop_front();
        ++locked_count;
        lock_holder = next;
        next->notify_sometime();
    }
}
