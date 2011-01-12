#ifndef __BUFFER_CACHE_CO_FUNCTIONS_HPP__
#define __BUFFER_CACHE_CO_FUNCTIONS_HPP__

// TODO: Some of this stuff (all of it?) actually belongs in buffer_cache/.

#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/large_buf.hpp"

// Avoid using this!  Use buf_lock_t instead.
buf_t *co_acquire_transaction(transaction_t *transaction, block_id_t block_id, access_t mode);

void co_acquire_large_value(large_buf_t *large_value, large_buf_ref root_ref_, access_t access_);

// Avoid using this, use transactor_t instead.
transaction_t *co_begin_transaction(cache_t *cache, access_t access);

// Avoid using this, use transactor_t instead.
void co_commit(transaction_t *transaction);


#endif  // __BUFFER_CACHE_CO_FUNCTIONS_HPP__
