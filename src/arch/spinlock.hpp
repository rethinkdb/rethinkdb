// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_SPINLOCK_HPP_
#define ARCH_SPINLOCK_HPP_

#include <pthread.h>
#include <string.h>

#include "errors.hpp"

// TODO(OSX) Reconsider how to detect spinlock feature.
#if __APPLE__
#define PTHREAD_HAS_SPINLOCK 0
#else
#define PTHREAD_HAS_SPINLOCK 1
#endif

// TODO: we should use regular mutexes on single core CPU
// instead of spinlocks

class spinlock_t {
public:
    friend class spinlock_acq_t;

    spinlock_t() {
#if PTHREAD_HAS_SPINLOCK
        pthread_spin_init(&l, PTHREAD_PROCESS_PRIVATE);
#else
        pthread_mutex_init(&l, NULL);
#endif
    }
    ~spinlock_t() {
#if PTHREAD_HAS_SPINLOCK
        pthread_spin_destroy(&l);
#else
        pthread_mutex_destroy(&l);
#endif
    }

private:
    void lock() {
#if PTHREAD_HAS_SPINLOCK
        int res = pthread_spin_lock(&l);
#else
        int res = pthread_mutex_lock(&l);
#endif
        guarantee_err(res == 0, "could not lock spin lock");
    }
    void unlock() {
#if PTHREAD_HAS_SPINLOCK
        int res = pthread_spin_unlock(&l);
#else
        int res = pthread_mutex_unlock(&l);
#endif
        guarantee_err(res == 0, "could not unlock spin lock");
    }

#if PTHREAD_HAS_SPINLOCK
    pthread_spinlock_t l;
#else
    pthread_mutex_t l;
#endif

    DISABLE_COPYING(spinlock_t);
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

    DISABLE_COPYING(spinlock_acq_t);
};

#endif /* ARCH_SPINLOCK_HPP_ */
