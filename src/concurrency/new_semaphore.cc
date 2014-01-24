#include "concurrency/new_semaphore.hpp"

new_semaphore_t::new_semaphore_t(int64_t capacity)
    : capacity_(capacity), current_(0) { }

new_semaphore_t::~new_semaphore_t() {
    guarantee(current_ == 0);
    guarantee(waiters_.empty());
}

void new_semaphore_t::add_acquirer(new_semaphore_acq_t *acq) {
    waiters_.push_back(acq);
    pulse_waiters();
}

void new_semaphore_t::remove_acquirer(new_semaphore_acq_t *acq) {
    rassert(acq->semaphore_ == this);
    if (acq->cond_.is_pulsed()) {
        current_ -= acq->count_;
    }
    pulse_waiters();
}

void new_semaphore_t::pulse_waiters() {
    while (new_semaphore_acq_t *acq = waiters_.head()) {
        if (acq->count_ <= capacity_ - current_) {
            current_ += acq->count_;
            waiters_.remove(acq);
            acq->cond_.pulse();
        } else {
            break;
        }
    }
}

new_semaphore_acq_t::~new_semaphore_acq_t() {
    if (semaphore_) {
        semaphore_->remove_acquirer(this);
    }
}

new_semaphore_acq_t::new_semaphore_acq_t()
    : semaphore_(NULL), count_(0) { }

new_semaphore_acq_t::new_semaphore_acq_t(new_semaphore_t *semaphore, int64_t count)
    : semaphore_(NULL), count_(0) {
    init(semaphore, count);
}

void new_semaphore_acq_t::init(new_semaphore_t *semaphore, int64_t count) {
    guarantee(semaphore_ == NULL);
    rassert(count_ == 0);
    guarantee(count >= 0);
    semaphore_ = semaphore;
    count_ = count;
    semaphore_->add_acquirer(this);
}


