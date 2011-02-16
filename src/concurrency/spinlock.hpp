#ifndef __CONCURRENCY_SPINLOCK_HPP__
#define __CONCURRENCY_SPINLOCK_HPP__

#include "arch/arch.hpp"

// Class that wraps a pthread spinlock.
class spinlock_t {
    pthread_spinlock_t l;
public:
    spinlock_t() {
        pthread_spin_init(&l, PTHREAD_PROCESS_PRIVATE);
    }
    ~spinlock_t() {
        pthread_spin_destroy(&l);
    }
    void lock() {
        int res = pthread_spin_lock(&l);
        guarantee_err(res == 0, "could not lock spin lock");
    }
    void unlock() {
        int res = pthread_spin_unlock(&l);
        guarantee_err(res == 0, "could not unlock spin lock");
    }
};

#endif // __CONCURRENCY_SPINLOCK_HPP__
