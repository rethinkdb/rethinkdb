#ifndef ARCH_BARRIER_HPP_
#define ARCH_BARRIER_HPP_

#include <pthread.h>

#include "errors.hpp"

// We call this a thread_barrier_t so as to differentiate from other barrier types.
class thread_barrier_t {
public:
    explicit thread_barrier_t(int num_workers);
    ~thread_barrier_t();

    void wait();

private:
    const int num_workers_;
    int num_waiters_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;

    DISABLE_COPYING(thread_barrier_t);
};

#endif  // ARCH_BARRIER_HPP_
