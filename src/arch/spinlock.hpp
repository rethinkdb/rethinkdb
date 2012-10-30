// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_SPINLOCK_HPP_
#define ARCH_SPINLOCK_HPP_

#include <pthread.h>
#include <string.h>

#include "errors.hpp"

class spinlock_t {
public:
    friend class spinlock_acq_t;

    spinlock_t() {
        pthread_spin_init(&l, PTHREAD_PROCESS_PRIVATE);
    }
    ~spinlock_t() {
        pthread_spin_destroy(&l);
    }

private:
    void lock() {
        int res = pthread_spin_lock(&l);
        guarantee_err(res == 0, "could not lock spin lock");
    }
    void unlock() {
        int res = pthread_spin_unlock(&l);
        guarantee_err(res == 0, "could not unlock spin lock");
    }

    pthread_spinlock_t l;
    spinlock_t(const spinlock_t&);
    void operator=(const spinlock_t&);
};

class spinlock_acq_t {
public:
    explicit spinlock_acq_t(spinlock_t *the_lock) : the_lock_(the_lock) {
        the_lock_->lock();
    }
    ~spinlock_acq_t() {
        the_lock_->unlock();
    }

private:
    spinlock_t *the_lock_;
    spinlock_acq_t(const spinlock_acq_t&);
    void operator=(const spinlock_acq_t&);
};

#endif /* ARCH_SPINLOCK_HPP_ */
