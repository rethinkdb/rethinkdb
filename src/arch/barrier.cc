#include "arch/barrier.hpp"

#if __APPLE__
// TODO(OSX) add unit tests for thread_barrier_t and run them on OS X.
thread_barrier_t::thread_barrier_t(int num_workers)
    : num_workers_(num_workers), num_waiters_(0) {
    guarantee(num_workers > 0);

    int res = pthread_mutex_init(&mutex_, NULL);
    guarantee(res == 0, "could not initialize barrier mutex");
    res = pthread_cond_init(&cond_, NULL);
    guarantee(res == 0, "could not initialize barrier condition variable");
}

thread_barrier_t::~thread_barrier_t() {
    guarantee(num_waiters_ == 0);
    int res = pthread_cond_destroy(&cond_);
    guarantee(res == 0, "could not destroy barrier condition variable");
    res = pthread_mutex_destroy(&mutex_);
    guarantee(res == 0, "could not destroy barrier mutex");
}

void thread_barrier_t::wait() {
    int res = pthread_mutex_lock(&mutex_);
    guarantee(res == 0, "could not acquire barrier mutex");
    ++num_waiters_;
    if (num_waiters_ == num_workers_) {
        num_waiters_ = 0;
        res = pthread_cond_broadcast(&cond_);
        guarantee(res == 0, "barrier could not broadcast to condition variable");
    } else {
        res = pthread_cond_wait(&cond_, &mutex_);
        guarantee(res == 0, "barrier could not wait for condition variable");
    }
    res = pthread_mutex_unlock(&mutex_);
    guarantee(res == 0, "barrier could not unlock mutex");
}


#else
thread_barrier_t::thread_barrier_t(int num_workers) {
    guarantee(num_workers > 0);
    int res = pthread_barrier_init(&barrier_, NULL, num_workers);
    guarantee(res == 0, "could not create barrier");
}

thread_barrier_t::~thread_barrier_t() {
    int res = pthread_barrier_destroy(&barrier_);
    guarantee(res == 0, "could not destroy barrier");
}

void thread_barrier_t::wait() {
    int res = pthread_barrier_wait(&barrier_);
    guarantee(res == 0 || res == PTHREAD_BARRIER_SERIAL_THREAD, "could not wait at barrier");
}
#endif  // __APPLE__
