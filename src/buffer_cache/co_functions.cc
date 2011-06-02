#include "buffer_cache/co_functions.hpp"
#include "concurrency/promise.hpp"

buf_t *co_acquire_block(transaction_t *transaction, block_id_t block_id, access_t mode, threadsafe_cond_t *acquisition_cond) {
    transaction->assert_thread();
    buf_t *value = transaction->acquire(block_id, mode, acquisition_cond ? boost::bind(&threadsafe_cond_t::pulse, acquisition_cond) : boost::function<void()>());
    rassert(value);
    return value;
}


struct large_value_acquired_t : public large_buf_available_callback_t {
    coro_t *self;
    large_value_acquired_t() : self(coro_t::self()) { }
    void on_large_buf_available(UNUSED large_buf_t *large_value) { self->notify(); }
};

void co_acquire_large_buf_for_unprepend(large_buf_t *lb, int64_t length) {
    large_value_acquired_t acquired;
    lb->assert_thread();
    lb->acquire_for_unprepend(length, &acquired);
    coro_t::wait();
}

void co_acquire_large_buf_slice(large_buf_t *lb, int64_t offset, int64_t size, threadsafe_cond_t *acquisition_cond) {
    large_value_acquired_t acquired;
    lb->assert_thread();
    lb->acquire_slice(offset, size, &acquired);
    if (acquisition_cond) {
        // TODO: This is worthless crap, because the
        // transaction_t::acquire interface is a lie.
        acquisition_cond->pulse();
    }
    coro_t::wait();
}

void co_acquire_large_buf(large_buf_t *lb, threadsafe_cond_t *acquisition_cond) {
    lb->assert_thread();
    co_acquire_large_buf_slice(lb, 0, lb->root_ref->size, acquisition_cond);
}

void co_acquire_large_buf_lhs(large_buf_t *lb) {
    lb->assert_thread();
    co_acquire_large_buf_slice(lb, 0, std::min(1L, lb->root_ref->size));
}

void co_acquire_large_buf_rhs(large_buf_t *lb) {
    lb->assert_thread();
    int64_t beg = std::max(int64_t(0), lb->root_ref->size - 1);
    co_acquire_large_buf_slice(lb, beg, lb->root_ref->size - beg);
}

void co_acquire_large_buf_for_delete(large_buf_t *large_value) {
    large_value_acquired_t acquired;
    large_value->acquire_for_delete(&acquired);
    coro_t::wait();
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

transaction_t *co_begin_transaction(cache_t *cache, access_t access, int expected_change_count, repli_timestamp recency_timestamp) {
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


void co_get_subtree_recencies(transaction_t *txn, block_id_t *block_ids, size_t num_block_ids, repli_timestamp *recencies_out) {
    struct : public get_subtree_recencies_callback_t {
        void got_subtree_recencies() { cond.pulse(); }
        cond_t cond;
    } cb;

    txn->get_subtree_recencies(block_ids, num_block_ids, recencies_out, &cb);

    cb.cond.wait();
}
