#include "concurrency/rwlock.hpp"

rwlock_acq_t::rwlock_acq_t(rwlock_t *lock,
                           rwlock_access_t access)
    : lock_(lock), access_(access) {
    lock_->add_acq(this);
}

rwlock_acq_t::~rwlock_acq_t() {
    lock_->remove_acq(this);
}

signal_t *rwlock_acq_t::rwlock_acq_signal() {
    return &cond_;
}

rwlock_t::rwlock_t() { }

rwlock_t::~rwlock_t() {
    guarantee(acqs_.empty(), "An rwlock_t was destroyed while locks are held.");
}

void rwlock_t::add_acq(rwlock_acq_t *acq) {
    ASSERT_NO_CORO_WAITING;
    acqs_.push_back(acq);

    pulse_chain(acq);
}

void rwlock_t::remove_acq(rwlock_acq_t *acq) {
    ASSERT_NO_CORO_WAITING;
    rwlock_acq_t *next = acqs_.next(acq);
    acqs_.remove(acq);
    if (next != NULL) {
        pulse_chain(next);
    }
}

void rwlock_t::pulse_chain(rwlock_acq_t *const acq) {
    // Pulses acq, and, if possible, pulses subsequent nodes.
    rwlock_acq_t *const prev = acqs_.prev(acq);
    if (prev == NULL && acq->access_ == rwlock_access_t::write) {
        // Give acq the write-lock.
        acq->cond_.pulse_if_not_already_pulsed();
    } else if (prev == NULL || (prev->access_ == rwlock_access_t::read
                                && prev->cond_.is_pulsed())) {
        // Give contiguous read-followers the read-lock.  (But if acq->cond_ was
        // already pulsed, then they already got it, so don't bother.)
        if (!acq->cond_.is_pulsed()) {
            rwlock_acq_t *node = acq;
            while (node != NULL
                   && node->access_ == rwlock_access_t::read) {
                node->cond_.pulse();
                node = acqs_.next(node);
            }
        }
    }
}
