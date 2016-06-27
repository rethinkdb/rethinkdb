#include "arch/barrier.hpp"

thread_barrier_t::thread_barrier_t(int num_workers)
    : num_workers_(num_workers), num_waiters_(0) {
    guarantee(num_workers > 0);

    int res = pthread_mutex_init(&mutex_, nullptr);
    guarantee_xerr(res == 0, res, "could not initialize barrier mutex");
    res = pthread_cond_init(&cond_, nullptr);
    guarantee_xerr(res == 0, res, "could not initialize barrier condition variable");
}

thread_barrier_t::~thread_barrier_t() {
    guarantee(num_waiters_ == 0);
    int res = pthread_cond_destroy(&cond_);
    guarantee_xerr(res == 0, res, "could not destroy barrier condition variable");
    res = pthread_mutex_destroy(&mutex_);
    guarantee_xerr(res == 0, res, "could not destroy barrier mutex");
}

void thread_barrier_t::wait() {
    int res = pthread_mutex_lock(&mutex_);
    guarantee_xerr(res == 0, res, "could not acquire barrier mutex");
    ++num_waiters_;
    if (num_waiters_ == num_workers_) {
        num_waiters_ = 0;
        res = pthread_cond_broadcast(&cond_);
        guarantee_xerr(res == 0, res, "barrier could not broadcast to condition variable");
    } else {
        res = pthread_cond_wait(&cond_, &mutex_);
        guarantee_xerr(res == 0, res, "barrier could not wait for condition variable");
    }
    res = pthread_mutex_unlock(&mutex_);
    guarantee_xerr(res == 0, res, "barrier could not unlock mutex");
}
