#include "arch/barrier.hpp"

#if __APPLE__
// TODO(OSX) add unit tests for thread_barrier_t and run them on OS X.
thread_barrier_t::thread_barrier_t(int num_workers)
    : num_workers_(num_workers), num_waiters_(0) {
    guarantee(num_workers > 0);

    int res = pthread_mutex_init(&mutex_, NULL);
    guarantee_xerr(res == 0, res, "could not initialize barrier mutex");
    res = pthread_cond_init(&cond_, NULL);
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


#else
// TODO(OSX) Just remove this pthread_barrier_t-using code?
thread_barrier_t::thread_barrier_t(int num_workers) {
    guarantee(num_workers > 0);
    // TODO(OSX) Check pthread_barrier_init man page (on linux) to make sure it returns an error code.
    int res = pthread_barrier_init(&barrier_, NULL, num_workers);
    guarantee_xerr(res == 0, res, "could not create barrier");
}

thread_barrier_t::~thread_barrier_t() {
    // TODO(OSX) Check pthread_barrier_destroy man page (on linux) to make sure it returns an error code.
    int res = pthread_barrier_destroy(&barrier_);
    guarantee_xerr(res == 0, res, "could not destroy barrier");
}

void thread_barrier_t::wait() {
    // TODO(OSX) Check pthread_barrier_wait man page (on linux) to make sure it returns an error code.
    int res = pthread_barrier_wait(&barrier_);
    guarantee_xerr(res == 0 || res == PTHREAD_BARRIER_SERIAL_THREAD, res, "could not wait at barrier");
}
#endif  // __APPLE__
