// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "concurrency/mutex.hpp"

#include "arch/runtime/coroutines.hpp"

mutex_t::acq_t::acq_t(mutex_t *l, bool eager) : lock_(nullptr), eager_(false) {
    reset(l, eager);
}

mutex_t::acq_t::~acq_t() {
    reset();
}

void mutex_t::acq_t::reset() {
    if (lock_) {
        unlock_mutex(lock_, eager_);
    }
    lock_ = nullptr;
}

void mutex_t::acq_t::reset(mutex_t *l, bool eager) {
    reset();
    lock_ = l;
    eager_ = eager;
    co_lock_mutex(l);
}

void co_lock_mutex(mutex_t *mutex) {
    if (mutex->locked) {
        mutex->waiters.push_back(coro_t::self());
        coro_t::wait();
    } else {
        mutex->locked = true;
    }
}

void unlock_mutex(mutex_t *mutex, bool eager) {
    rassert(mutex->locked);
    if (mutex->waiters.empty()) {
        mutex->locked = false;
    } else {
        coro_t *next = mutex->waiters.front();
        mutex->waiters.pop_front();
        if (eager) {
            next->notify_now_deprecated();
        } else {
            next->notify_sometime();
        }
    }
}
