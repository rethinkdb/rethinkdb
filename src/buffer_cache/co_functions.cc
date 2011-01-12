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

buf_t *co_acquire_transaction(transaction_t *transaction, block_id_t block_id, access_t mode) {
    co_block_available_callback_t cb;
    buf_t *value = transaction->acquire(block_id, mode, &cb);
    if (!value) {
        value = cb.join();
    }
    assert(value);
    return value;
}





struct large_value_acquired_t : public large_buf_available_callback_t {
    coro_t *self;
    large_value_acquired_t() : self(coro_t::self()) { }
    void on_large_buf_available(large_buf_t *large_value) { self->notify(); }
};

void co_acquire_large_value(large_buf_t *large_value, large_buf_ref root_ref_, access_t access_) {
    large_value_acquired_t acquired;
    large_value->acquire(root_ref_, access_, &acquired);
    coro_t::wait();
}
