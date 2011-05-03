#include "concurrency/coro_fifo.hpp"

void coro_fifo_t::inform_ready_to_exit(coro_fifo_acq_t *acq) {
    assert_thread();

    acq->is_waiting_to_exit_ = true;
    coro_fifo_acq_t *head = acquisitor_queue_.head();
    rassert(head, "there must be a head node because somebody in the queue called this function");
    while (head && head->is_waiting_to_exit_) {
        head->ready_to_exit_.pulse();
        acquisitor_queue_.remove(head);
    }
}
