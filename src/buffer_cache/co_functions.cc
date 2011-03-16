#include "buffer_cache/co_functions.hpp"

struct co_block_available_callback_t : public block_available_callback_t {
    coro_t *self;
    buf_t *value;

    virtual void on_block_available(buf_t *block) {
        value = block;
        self->notify();
    }

    buf_t *join() {
        self = coro_t::self();
        coro_t::wait();
        return value;
    }
};

buf_t *co_acquire_block(transaction_t *transaction, block_id_t block_id, access_t mode, cond_t *acquisition_cond) {
    transaction->ensure_thread();
    co_block_available_callback_t cb;
    buf_t *value = transaction->acquire(block_id, mode, &cb, acquisition_cond);
    if (!value) {
        value = cb.join();
    }
    rassert(value);
    return value;
}


struct large_value_acquired_t : public large_buf_available_callback_t {
    coro_t *self;
    large_value_acquired_t() : self(coro_t::self()) { }
    void on_large_buf_available(large_buf_t *large_value) { self->notify(); }
};

void co_acquire_large_buf_for_unprepend(large_buf_t *lb, int64_t length) {
    large_value_acquired_t acquired;
    lb->ensure_thread();
    lb->acquire_for_unprepend(length, &acquired);
    coro_t::wait();
}

void co_acquire_large_buf_slice(large_buf_t *lb, int64_t offset, int64_t size, cond_t *acquisition_cond) {
    large_value_acquired_t acquired;
    lb->ensure_thread();
    lb->acquire_slice(offset, size, &acquired);
    acquisition_cond->pulse();
    coro_t::wait();
}

void co_acquire_large_buf(large_buf_t *lb, cond_t *acquisition_cond) {
    co_acquire_large_buf_slice(lb, 0, lb->root_ref->size, acquisition_cond);
}

void co_acquire_large_buf_lhs(large_buf_t *lb) {
    co_acquire_large_buf_slice(lb, 0, std::min(1L, lb->root_ref->size));
}

void co_acquire_large_buf_rhs(large_buf_t *lb) {
    int64_t beg = std::max(int64_t(0), lb->root_ref->size - 1);
    co_acquire_large_buf_slice(lb, beg, lb->root_ref->size - beg);
}

void co_acquire_large_buf_for_delete(large_buf_t *large_value) {
    large_value_acquired_t acquired;
    large_value->acquire_for_delete(&acquired);
    coro_t::wait();
}

// Well this is repetitive.
struct transaction_begun_callback_t : public transaction_begin_callback_t {
    coro_t *self;
    transaction_t *value;

    void on_txn_begin(transaction_t *txn) {
        value = txn;
        self->notify();
    }

    transaction_t *join() {
        self = coro_t::self();
        coro_t::wait();
        return value;
    }
};

transaction_t *co_begin_transaction(cache_t *cache, access_t access, repli_timestamp recency_timestamp) {
    cache->ensure_thread();
    transaction_begun_callback_t cb;
    transaction_t *value = cache->begin_transaction(access, &cb, recency_timestamp);
    if (!value) {
        value = cb.join();
    }
    rassert(value);
    return value;
}



struct transaction_committed_t : public transaction_commit_callback_t {
    coro_t *self;
    transaction_committed_t() : self(coro_t::self()) { }
    void on_txn_commit(transaction_t *transaction) {
        self->notify();
    }
};


void co_commit_transaction(transaction_t *transaction) {
    transaction->ensure_thread();
    transaction_committed_t cb;
    if (!transaction->commit(&cb)) {
        coro_t::wait();
    }
}
