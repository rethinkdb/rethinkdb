#include "btree/coro_wrappers.hpp"

bool co_get_data_provider_value(data_provider_t *data, buffer_group_t *dest) {
    co_data_provider_done_callback_t cb;
    data->get_value(dest, &cb);
    return cb.join();
}

void co_value(store_t::get_callback_t *cb, const_buffer_group_t *value_buffers, mcflags_t flags, cas_t cas) {
    value_done_t done;
    cb->value(value_buffers, &done, flags, cas);
    coro_t::wait();
}

transaction_t *co_begin_transaction(cache_t *cache, access_t access) {
    co_transaction_begin_callback_t cb;
    transaction_t *value = cache->begin_transaction(access, &cb);
    // XXX: Verify that read-only transactions begin immediately?
    if (!value) value = cb.join();
    assert(value);
    return value;
}

void co_acquire_large_value(large_buf_t *large_value, large_buf_ref root_ref_, access_t access_) {
    co_large_buf_available_callback_t cb;
    large_value->acquire(root_ref_, access_, &cb);
    coro_t::wait();
}

void co_acquire_large_value_lhs(large_buf_t *large_value, large_buf_ref root_ref_, access_t access_) {
    co_large_buf_available_callback_t cb;
    large_value->acquire_lhs(root_ref_, access_, &cb);
    coro_t::wait();
}

void co_acquire_large_value_rhs(large_buf_t *large_value, large_buf_ref root_ref_, access_t access_) {
    co_large_buf_available_callback_t cb;
    large_value->acquire_rhs(root_ref_, access_, &cb);
    coro_t::wait();
}

buf_t *co_acquire_block(transaction_t *transaction, block_id_t block_id, access_t mode) {
    co_block_available_callback_t cb;
    buf_t *value = transaction->acquire(block_id, mode, &cb);
    if (!value) value = cb.join();
    return value;
}

void co_commit_transaction(transaction_t *txn) {
    co_transaction_commit_callback_t cb;
    bool success = txn->commit(&cb);
    if (!success) cb.join();
}

// XXX
//void co_replicant_value(store_t::replicant_t *replicant, const store_key_t *key,
//                        const_buffer_group_t *value, mcflags_t flags,
//                        exptime_t exptime, cas_t cas, repli_timestamp timestamp) {
//    co_replicant_done_callback_t cb;
//    replicant->value(key, value, &cb, flags, exptime, cas, timestamp);
//    cb.join();
//}
