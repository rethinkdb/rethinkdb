#include "buffer_cache/co_functions.hpp"
#include "concurrency/promise.hpp"

buf_t *co_acquire_block(transaction_t *transaction, block_id_t block_id, access_t mode, threadsafe_cond_t *acquisition_cond) {
    transaction->assert_thread();
    buf_t *value = transaction->acquire(block_id, mode, acquisition_cond ? boost::bind(&threadsafe_cond_t::pulse, acquisition_cond) : boost::function<void()>());
    rassert(value);
    return value;
}


/*

  TODO: Merge the coro_fifo_t change into whatever replaced our
  co_begin_transaction.

// Well this is repetitive.
struct transaction_begun_callback_t : public transaction_begin_callback_t {
    flat_promise_t<transaction_t*> txn;

    void on_txn_begin(transaction_t *txn_) {
        txn.pulse(txn_);
    }

    transaction_t *join() {
        return txn.wait();
    }
};

transaction_t *co_begin_transaction(cache_t *cache, access_t access, int expected_change_count, repli_timestamp_t recency_timestamp) {
    cache->assert_thread();
    transaction_begun_callback_t cb;

    // Writes and reads must separately have their order preserved,
    // but we're expecting, in the case of throttling, for writes to
    // be throttled while reads are not.

    // We think there is a bug in throttling that could reorder writes
    // when throttling _ends_.  Reordering can also happen if
    // begin_transaction sometimes returns NULL and other times does
    // not, because then in one case cb.join() needs to spin around
    // the event loop.  So there's an obvious problem with the code
    // below, if we did not have coro_fifo_acq_t protecting it.

    coro_fifo_acq_t acq;
    if (is_write_mode(access)) {
        // We only care about write ordering and not anything about
        // write operations' interaction with read ordering because
        // that's how throttling works right now.
        acq.enter(&cache->co_begin_coro_fifo());
    }

    transaction_t *value = cache->begin_transaction(access, expected_change_count, recency_timestamp, &cb);
    if (!value) {
        value = cb.join();
    }

    rassert(value);
    return value;
}
*/


void co_get_subtree_recencies(transaction_t *txn, block_id_t *block_ids, size_t num_block_ids, repli_timestamp_t *recencies_out) {
    struct : public get_subtree_recencies_callback_t {
        void got_subtree_recencies() { cond.pulse(); }
        cond_t cond;
    } cb;

    txn->get_subtree_recencies(block_ids, num_block_ids, recencies_out, &cb);

    cb.cond.wait();
}
