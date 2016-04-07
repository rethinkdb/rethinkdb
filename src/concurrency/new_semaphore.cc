#include "concurrency/new_semaphore.hpp"

new_semaphore_t::new_semaphore_t(int64_t capacity)
    : capacity_(capacity), current_(0)
{
    guarantee(capacity > 0);
}

new_semaphore_t::~new_semaphore_t() {
    guarantee(current_ == 0);
    guarantee(waiters_.empty());
}

void new_semaphore_t::set_capacity(int64_t new_capacity) {
    assert_thread();
    guarantee(new_capacity > 0);
    capacity_ = new_capacity;
    pulse_waiters();
}

void new_semaphore_t::add_acquirer(new_semaphore_in_line_t *acq) {
    assert_thread();
    waiters_.push_back(acq);
    pulse_waiters();
}

void new_semaphore_t::remove_acquirer(new_semaphore_in_line_t *acq) {
    assert_thread();
    rassert(acq->semaphore_ == this);
    if (acq->cond_.is_pulsed()) {
        current_ -= acq->count_;
    } else {
        waiters_.remove(acq);
    }
    pulse_waiters();
}

void new_semaphore_t::pulse_waiters() {
    assert_thread();
    while (new_semaphore_in_line_t *acq = waiters_.head()) {
        if (acq->count_ <= capacity_ - current_ || current_ == 0) {
            current_ += acq->count_;
            waiters_.remove(acq);
            acq->cond_.pulse();
        } else {
            break;
        }
    }
}

new_semaphore_in_line_t::~new_semaphore_in_line_t() {
    reset();
}

void new_semaphore_in_line_t::reset() {
    if (semaphore_ != nullptr) {
        semaphore_->remove_acquirer(this);
        semaphore_ = nullptr;
        count_ = 0;
        cond_.reset();
    }
}

new_semaphore_in_line_t::new_semaphore_in_line_t()
    : semaphore_(nullptr), count_(0) { }

new_semaphore_in_line_t::new_semaphore_in_line_t(new_semaphore_t *semaphore, int64_t count)
    : semaphore_(nullptr), count_(0) {
    init(semaphore, count);
}

void new_semaphore_in_line_t::init(new_semaphore_t *semaphore, int64_t count) {
    guarantee(semaphore_ == nullptr);
    rassert(count_ == 0);
    guarantee(count >= 0);
    semaphore_ = semaphore;
    count_ = count;
    semaphore_->add_acquirer(this);
}

new_semaphore_in_line_t::new_semaphore_in_line_t(new_semaphore_in_line_t &&movee)
    : intrusive_list_node_t<new_semaphore_in_line_t>(std::move(movee)),
      semaphore_(movee.semaphore_),
      count_(movee.count_),
      cond_(std::move(movee.cond_)) {
    movee.semaphore_ = nullptr;
    movee.count_ = 0;
    movee.cond_.reset();
}

int64_t new_semaphore_in_line_t::count() const {
    return count_;
}

void new_semaphore_in_line_t::change_count(int64_t new_count) {
    guarantee(semaphore_ != nullptr);
    guarantee(new_count >= 0);

    if (cond_.is_pulsed()) {
        // We've already acquired the semaphore, which means semaphore_->current_
        // already accounts for our existing count_ amount of acquisition, now
        // add/subtract the difference.
        semaphore_->current_ += (new_count - count_);
    }

    count_ = new_count;

    // Things have changed, tell the semaphore to try pulsing waiters (ourself, if we
    // haven't acquired it yet, or other waiters, if we have reduced current_).
    semaphore_->pulse_waiters();
}

void new_semaphore_in_line_t::transfer_in(new_semaphore_in_line_t &&other) {
    guarantee(semaphore_ != nullptr);
    guarantee(other.semaphore_ == semaphore_);
    guarantee(cond_.is_pulsed());
    guarantee(other.cond_.is_pulsed());
    change_count(count_ + other.count_);
    other.reset();
}

