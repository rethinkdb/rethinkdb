// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "concurrency/coro_fifo.hpp"

// This is an inefficient implementation but we'd need extreme hacks
// or coro_t magic to make this efficient.  This works efficiently
// with no blocking in the case where no reordering is needed.

void coro_fifo_t::inform_ready_to_leave(coro_fifo_acq_t *acq) {
    assert_thread();

    acq->asked_to_leave_ = true;

    consider_pulse();
}

void coro_fifo_t::consider_pulse() {
    if (queue_counter_ == 0) {
        coro_fifo_acq_t *head = acquisitor_queue_.head();

        if (head && head->asked_to_leave_) {
            ++queue_counter_;
            acquisitor_queue_.remove(head);
            head->ready_to_leave_.pulse();
        }
    }
}

void coro_fifo_t::inform_left_and_notified() {
    --queue_counter_;
    consider_pulse();
}

coro_fifo_acq_t::coro_fifo_acq_t() : initialized_(false), fifo_(NULL), asked_to_leave_(false) { }

coro_fifo_acq_t::~coro_fifo_acq_t() {
    if (initialized_) {
        leave();
    }
}

void coro_fifo_acq_t::enter(coro_fifo_t *fifo) {
    rassert(!initialized_);
    rassert(!fifo_);
    initialized_ = true;
    fifo_ = fifo;
    asked_to_leave_ = false;
    fifo_->assert_thread();
    fifo_->acquisitor_queue_.push_back(this);
}

void coro_fifo_acq_t::leave() {
    rassert(initialized_);
    if (initialized_) {
        fifo_->inform_ready_to_leave(this);
        ready_to_leave_.wait();
        fifo_->inform_left_and_notified();
        rassert(!in_a_list(), "should have been removed from the acquisitor_queue_");
        initialized_ = false;
    }
}
