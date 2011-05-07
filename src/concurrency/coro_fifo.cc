#include "concurrency/coro_fifo.hpp"

void coro_fifo_t::inform_ready_to_leave(coro_fifo_acq_t *acq) {
    assert_thread();

    acq->asked_to_leave_ = true;
    coro_fifo_acq_t *head = acquisitor_queue_.head();
    rassert(head, "there must be a head node because somebody in the queue called this function");
    while (head && head->asked_to_leave_) {
        head->ready_to_leave_.pulse();
        acquisitor_queue_.remove(head);
    }
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
        rassert(!in_a_list, "should have been removed from the acquisitor_queue_");
        initialized_ = false;
    }
}
