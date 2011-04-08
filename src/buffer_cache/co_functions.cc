#include "buffer_cache/co_functions.hpp"

struct large_value_acquired_t : public large_buf_available_callback_t {
    coro_t *self;
    large_value_acquired_t() : self(coro_t::self()) { }
    void on_large_buf_available(UNUSED large_buf_t *large_value) { self->notify(); }
};

void co_acquire_large_buf_for_unprepend(const thread_saver_t& saver, large_buf_t *lb, int64_t length) {
    large_value_acquired_t acquired;
    lb->ensure_thread(saver);
    lb->acquire_for_unprepend(length, &acquired);
    coro_t::wait();
}

void co_acquire_large_buf_slice(const thread_saver_t& saver, large_buf_t *lb, int64_t offset, int64_t size, threadsafe_cond_t *acquisition_cond) {
    large_value_acquired_t acquired;
    lb->ensure_thread(saver);
    lb->acquire_slice(offset, size, &acquired);
    if (acquisition_cond) {
        acquisition_cond->pulse();
    }
    coro_t::wait();
}

void co_acquire_large_buf(const thread_saver_t& saver, large_buf_t *lb, threadsafe_cond_t *acquisition_cond) {
    co_acquire_large_buf_slice(saver, lb, 0, lb->root_ref->size, acquisition_cond);
}

void co_acquire_large_buf_lhs(const thread_saver_t& saver, large_buf_t *lb) {
    co_acquire_large_buf_slice(saver, lb, 0, std::min(1L, lb->root_ref->size));
}

void co_acquire_large_buf_rhs(const thread_saver_t& saver, large_buf_t *lb) {
    int64_t beg = std::max(int64_t(0), lb->root_ref->size - 1);
    co_acquire_large_buf_slice(saver, lb, beg, lb->root_ref->size - beg);
}

void co_acquire_large_buf_for_delete(large_buf_t *large_value) {
    large_value_acquired_t acquired;
    large_value->acquire_for_delete(&acquired);
    coro_t::wait();
}

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

transaction_t *co_begin_transaction(const thread_saver_t& saver, cache_t *cache, access_t access, int expected_change_count, repli_timestamp recency_timestamp) {
    // TODO: ensure_thread is retarded.
    cache->ensure_thread(saver);
    transaction_begun_callback_t cb;
    transaction_t *value = cache->begin_transaction(access, expected_change_count, recency_timestamp, &cb);
    if (!value) {
        value = cb.join();
    }
    rassert(value);
    return value;
}



struct transaction_committed_t : public transaction_commit_callback_t {
    coro_t *self;
    transaction_committed_t() : self(coro_t::self()) { }
    void on_txn_commit(UNUSED transaction_t *transaction) {
        self->notify();
    }
};


void co_commit_transaction(const thread_saver_t& saver, transaction_t *transaction) {
    transaction->ensure_thread(saver);
    transaction_committed_t cb;
    if (!transaction->commit(&cb)) {
        coro_t::wait();
    }
}
