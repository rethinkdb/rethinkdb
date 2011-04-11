#include "replication/queueing_store.hpp"
#include "server/key_value_store.hpp"

namespace replication {

queueing_store_t::queueing_store_t(btree_key_value_store_t *inner) :
    inner_(inner), queue_mode_(queue_mode_on), queued_time_barrier_(repli_timestamp::invalid) {

    debugf("%p->queueing_store_t()\n", this);
}

queueing_store_t::~queueing_store_t() {
    debugf("%p->~queueing_store_t()\n", this);
    while (!mutation_queue_.empty()) {
        delete mutation_queue_.front().first;
        mutation_queue_.pop();
    }
}

mutation_result_t queueing_store_t::bypass_change(const mutation_t& m) {
    rassert(queue_mode_ == queue_mode_off);
    return inner_->change(m);
}

// HEY: The flushing logic here is quite fragile, since it makes some
// assumptions that we stay on the same coroutine between certain
// operations.

// TODO: Add locks to make these atomic operations explicitly atomic.

void queueing_store_t::handover(mutation_t *m, castime_t castime) {
    debugf("%p->handover()\n", this);
    // Atomic operation: pushing onto the queue and checking if
    // queue_mode_ in perhaps_flush, then starting flush.
    mutation_queue_.push(std::make_pair(m, castime));
    perhaps_flush();
}

void queueing_store_t::time_barrier(repli_timestamp timestamp) {
    if (queue_mode_ == queue_mode_off) {
        inner_->time_barrier(timestamp);
    } else {
        rassert(queued_time_barrier_ == repli_timestamp_t::invalid || queued_time_barrier_ < timestamp);
        queued_time_barrier_ = timestamp;
    }
}

void queueing_store_t::backfill_handover(mutation_t *m, castime_t castime) {
    inner_->change(*m, castime);
    delete m;
}

void queueing_store_t::backfill_complete(repli_timestamp timestamp) {
    inner_->time_barrier(timestamp);
    start_flush();
}

void queueing_store_t::perhaps_flush() {
    if (queue_mode_ == queue_mode_off) {
        // Atomic operation: Nothing can happen between seeing that
        // queue_mode_ is off and setting it to flushing inside
        // start_flush.
        start_flush();
    }
}

void queueing_store_t::start_flush() {
    // Callers (such as perhaps_flush) rely on the fact that no
    // coroutines can interrupt before we set the queue mode here to
    // 'queue_mode_flushing'.
    queue_mode_ = queue_mode_flushing;

    // Nothing can happen (no other coroutines can interrupt) between
    // seeing that the mutation_queue_ is empty and setting
    // queue_mode_ to 'queue_mode_off'.

    while (!mutation_queue_.empty()) {
        // TODO: Do these changes have the noreply property?  They should.
        inner_->change(*mutation_queue_.front().first, mutation_queue_.front().second);
        delete mutation_queue_.front().first;
        mutation_queue_.pop();
    }
    queue_mode_ = queue_mode_off;

    if (queued_time_barrier_.time != repli_timestamp::invalid.time) {
        inner_->time_barrier(queued_time_barrier_);
        queued_time_barrier_ = repli_timestamp::invalid;
    }
}


}  // namespace replication
