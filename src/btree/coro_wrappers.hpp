#ifndef __BTREE_CORO_WRAPPERS_HPP__
#define __BTREE_CORO_WRAPPERS_HPP__
// XXX This file is temporary.

#include "store.hpp"
#include "buffer_cache/large_buf.hpp"
#include "buffer_cache/buffer_cache.hpp"

// co_get_data_provider_value()
struct co_data_provider_done_callback_t : public data_provider_t::done_callback_t {
    coro_t *self;
    bool success;

    // Calling notify() on an active coroutine should probably be an error, but
    // it works for this, for now.

    co_data_provider_done_callback_t() : self(coro_t::self()) {}

    void have_provided_value() {
        success = true;
        self->notify();
    }

    void have_failed() {
        success = false;
        self->notify();
    }

    bool join() {
        coro_t::wait();
        return success;
    }
};

bool co_get_data_provider_value(data_provider_t *data, buffer_group_t *dest);

// co_value()
struct value_done_t : public store_t::get_callback_t::done_callback_t {
    coro_t *self;
    value_done_t() : self(coro_t::self()) { }
    void have_copied_value() { self->notify(); }
};

void co_value(store_t::get_callback_t *cb, const_buffer_group_t *value_buffers, mcflags_t flags, cas_t cas);

// co_acquire_large_value()
struct co_large_buf_available_callback_t : public large_buf_available_callback_t {
    coro_t *self;
    co_large_buf_available_callback_t() : self(coro_t::self()) { }
    void on_large_buf_available(large_buf_t *large_value) { self->notify(); }
};

void co_acquire_large_value(large_buf_t *large_value, large_buf_ref root_ref_, access_t access_);
void co_acquire_large_value_lhs(large_buf_t *large_value, large_buf_ref root_ref_, access_t access_);
void co_acquire_large_value_rhs(large_buf_t *large_value, large_buf_ref root_ref_, access_t access_);

// co_acquire_block()
struct co_block_available_callback_t : public block_available_callback_t {
    coro_t *self;
    buf_t *value;

    co_block_available_callback_t() : self(coro_t::self()) {}

    virtual void on_block_available(buf_t *block) {
        value = block;
        self->notify();
    }

    buf_t *join() {
        coro_t::wait();
        return value;
    }
};

buf_t *co_acquire_block(transaction_t *transaction, block_id_t block_id, access_t mode);

// co_begin_transaction()
struct co_transaction_begin_callback_t : public transaction_begin_callback_t {
    coro_t *self;
    transaction_t *value;

    co_transaction_begin_callback_t() : self(coro_t::self()) {}

    void on_txn_begin(transaction_t *txn) {
        value = txn;
        self->notify();
    }

    transaction_t *join() {
        coro_t::wait();
        return value;
    }
};

transaction_t *co_begin_transaction(cache_t *cache, access_t access);

// co_commit_transaction()
struct co_transaction_commit_callback_t : public transaction_commit_callback_t {
    coro_t *self;

    co_transaction_commit_callback_t() : self(coro_t::self()) {}

    void on_txn_commit(transaction_t *txn) {
        // TODO: Why does this get the transaction as an argument? That should
        // probably just be removed.
        self->notify();
    }

    void join() { coro_t::wait(); }
};

void co_commit_transaction(transaction_t *txn);

// XXX
// co_replicant_value()
//struct co_replicant_done_callback_t : public store_t::replicant_t::done_callback_t {
//    coro_t *self;
//    co_replicant_done_callback_t() : self(coro_t::self()) {}
//    void have_copied_value() { self->notify(); }
//    void join() { coro_t::wait(); }
//};
//
//void co_replicant_value(store_t::replicant_t *replicant, const store_key_t *key,
//                        const_buffer_group_t *value, mcflags_t flags,
//                        exptime_t exptime, cas_t cas, repli_timestamp timestamp);

#endif /* __BTREE_CORO_WRAPPERS_HPP__ */
