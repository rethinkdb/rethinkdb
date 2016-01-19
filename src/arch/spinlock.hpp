// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_SPINLOCK_HPP_
#define ARCH_SPINLOCK_HPP_

#include <pthread.h>
#include <string.h>

#include "errors.hpp"

// I don't know the "clean" way to detect the availability of the pthread
// spinlock feature.  For now we just use __MACH__ to default to mutex (which
// will just work) on Apple systems.  If it's ever useful, making our own
// spinlock implementation could be an option.
#if defined(__MACH__)
#define SPINLOCK_PTHREAD_MUTEX
#else
#define SPINLOCK_PTHREAD_SPINLOCK
#endif

// TODO: we should use regular mutexes on single core CPU
// instead of spinlocks

class spinlock_t {
public:
    friend class spinlock_acq_t;

    spinlock_t();
    ~spinlock_t();

private:
    void lock();
    void unlock();

#if defined(SPINLOCK_PTHREAD_SPINLOCK)
    pthread_spinlock_t l;
#elif defined (SPINLOCK_PTHREAD_MUTEX)
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
