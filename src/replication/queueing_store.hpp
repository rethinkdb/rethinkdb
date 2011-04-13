#ifndef __REPLICATION_QUEUEING_STORE_HPP__
#define __REPLICATION_QUEUEING_STORE_HPP__

#include <queue>

#include "store.hpp"

class btree_key_value_store_t;

namespace replication {

// TODO: Handle data provider lifetimes.

class queueing_store_t {
public:
    queueing_store_t(btree_key_value_store_t *inner);
    ~queueing_store_t();

    mutation_result_t bypass_change(const mutation_t& m);

    void handover(mutation_t *m, castime_t castime);
    void time_barrier(repli_timestamp timestamp);

    void backfill_handover(mutation_t *m, castime_t castime);
    void backfill_complete(repli_timestamp timestamp);

    btree_key_value_store_t *inner() { return inner_; }

private:
    void perhaps_flush();
    void start_flush();

    btree_key_value_store_t *inner_;
    enum { queue_mode_on, queue_mode_flushing, queue_mode_off };
    int queue_mode_;
    repli_timestamp queued_time_barrier_;
    // TODO: std::queue is an unacceptable data structure unless it
    // has better than ~(n) worst case.
    std::queue<std::pair<mutation_t *, castime_t> > mutation_queue_;

    DISABLE_COPYING(queueing_store_t);
};



}  // namespace replication


#endif  // __REPLICATION_QUEUEING_STORE_HPP__
