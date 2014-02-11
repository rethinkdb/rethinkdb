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
        if (mutex->locked) {
            mutex->waiters.push_back(coro_t::self());
            do_wait = true;
        } else {
            mutex->locked = true;
        }
    }
    if (do_wait) {
        coro_t::wait();
    }
}

void unlock_mutex(cross_thread_mutex_t *mutex) {
    spinlock_acq_t acq(&mutex->spinlock);
    rassert(mutex->locked);
    if (mutex->waiters.empty()) {
        mutex->locked = false;
    } else {
        coro_t *next = mutex->waiters.front();
        mutex->waiters.pop_front();
        next->notify_sometime();
    }
}
