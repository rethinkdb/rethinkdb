// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "concurrency/cross_thread_mutex.hpp"

#include "arch/runtime/coroutines.hpp"


cross_thread_mutex_t::acq_t::acq_t(cross_thread_mutex_t *l) : lock_(NULL) {
    reset(l);
}

cross_thread_mutex_t::acq_t::~acq_t() {
    reset();
}

void cross_thread_mutex_t::acq_t::reset() {
    if (lock_) {
        unlock_mutex(lock_);
    }
    lock_ = NULL;
}

void cross_thread_mutex_t::acq_t::reset(cross_thread_mutex_t *l) {
    reset();
    lock_ = l;
    co_lock_mutex(l);
}

void co_lock_mutex(cross_thread_mutex_t *mutex) {
    bool do_wait = false;
    {
        spinlock_acq_t acq(&mutex->spinlock);
        if (mutex->locked_count > 0 && mutex->lock_holder != coro_t::self()) {
            mutex->waiters.push_back(coro_t::self());
            do_wait = true;
        } else {
            guarantee(mutex->locked_count == 0 || mutex->is_recursive,
                "Deadlock detected. A coroutine tried to acquire a mutex which it is "
                "already holding.");
            mutex->lock_holder = coro_t::self();
            ++mutex->locked_count;
        }
    }
    if (do_wait) {
        coro_t::wait();
    }
}

void unlock_mutex(cross_thread_mutex_t *mutex) {
    spinlock_acq_t acq(&mutex->spinlock);
    rassert(mutex->locked_count > 0);
    rassert(mutex->lock_holder == coro_t::self());
    --mutex->locked_count;
    if (mutex->locked_count == 0 && !mutex->waiters.empty()) {
        coro_t *next = mutex->waiters.front();
        mutex->waiters.pop_front();
        ++mutex->locked_count;
        mutex->lock_holder = next;
        next->notify_sometime();
    }
}
