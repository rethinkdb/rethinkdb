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

void co_get_subtree_recencies(transaction_t *txn, block_id_t *block_ids, size_t num_block_ids, repli_timestamp *recencies_out) {
    struct : public get_subtree_recencies_callback_t {
        void got_subtree_recencies() { cond.pulse(); }
        cond_t cond;
    } cb;

    txn->get_subtree_recencies(block_ids, num_block_ids, recencies_out, &cb);

    cb.cond.wait();
}
