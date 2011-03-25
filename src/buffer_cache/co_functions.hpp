#ifndef __BUFFER_CACHE_CO_FUNCTIONS_HPP__
#define __BUFFER_CACHE_CO_FUNCTIONS_HPP__

#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/large_buf.hpp"

// Avoid using this!  Use buf_lock_t instead.
buf_t *co_acquire_block(transaction_t *transaction, block_id_t block_id, access_t mode, cond_t *acquisition_cond = NULL);

// TODO: Make acquisition_cond not take a default value, because I bet
// we should use it everywhere.  And put it on all of these functions.
//
// acquisition_cond gets pulsed immediately after a large buf
// acquisition was requested.  Its pulsing means that we could release
// the leaf node that owns it (assuming the large_buf isn't tied to
// the ref stored directly within the node.  But that would be
// improper).

void co_acquire_large_buf_for_unprepend(large_buf_t *lb, int64_t length);
void co_acquire_large_buf_slice(large_buf_t *lb, int64_t offset, int64_t size, cond_t *acquisition_cond = NULL);
void co_acquire_large_buf(large_buf_t *large_value, cond_t *acquisition_cond = NULL);
void co_acquire_large_buf_lhs(large_buf_t *large_value);
void co_acquire_large_buf_rhs(large_buf_t *large_value);
void co_acquire_large_buf_for_delete(large_buf_t *large_value);

// Avoid using this, use transactor_t instead.
transaction_t *co_begin_transaction(cache_t *cache, access_t access, int expected_change_count, repli_timestamp recency_timestamp);

// Avoid using this, use transactor_t instead.
void co_commit_transaction(transaction_t *transaction);


#endif  // __BUFFER_CACHE_CO_FUNCTIONS_HPP__
