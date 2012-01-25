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
         rassert(lock_->locked);
        if (lock_->waiters.empty()) {
            lock_->locked = false;
        } else {
            coro_t *next = lock_->waiters.front();
            lock_->waiters.pop_front();
            if (eager_) {
                next->notify_now();
            } else {
                next->notify_sometime();
            }
        }
    }
    lock_ = NULL;
}

void mutex_t::acq_t::reset(mutex_t *l, bool eager) {
    reset();
    lock_ = l;
    eager_ = eager;
    if (lock_->locked) {
        lock_->waiters.push_back(coro_t::self());
        coro_t::wait();
    } else {
        lock_->locked = true;
    }
}
