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

