#include "arch/barrier.hpp"

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
