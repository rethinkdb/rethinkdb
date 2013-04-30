// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/runtime.hpp"
#include "concurrency/mutex.hpp"

mutex_t::acq_t::acq_t(mutex_t *l, bool eager) : lock_(NULL), eager_(false) {
    reset(l, eager);
}

mutex_t::acq_t::~acq_t() {
    reset();
}

void mutex_t::acq_t::reset() {
    if (lock_) {
        unlock_mutex(lock_, eager_);
    }
    lock_ = NULL;
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
        if (mutex->waiters.empty()) {
            coro_t::wait(mutex->currently_running_coro);
        } else {
            coro_t::wait(mutex->waiters.back());
        }
    } else {
        mutex->locked = true;
        mutex->currently_running_coro = coro_t::self();
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
            mutex->currently_running_coro = next;
        } else {
            next->notify_sometime();
            mutex->currently_running_coro = next;
        }
    }
}
