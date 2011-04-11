#ifndef __BUFFER_CACHE_CO_FUNCTIONS_HPP__
#define __BUFFER_CACHE_CO_FUNCTIONS_HPP__

#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/large_buf.hpp"

// Avoid using this!  Use buf_lock_t instead.
buf_t *co_acquire_block(const thread_saver_t& saver, transaction_t *transaction, block_id_t block_id, access_t mode, threadsafe_cond_t *acquisition_cond = NULL);

// TODO: Make acquisition_cond not take a default value, because I bet
// we should use it everywhere.  And put it on all of these functions.
//
// acquisition_cond gets pulsed immediately after a large buf
// acquisition was requested.  Its pulsing means that we could release
// the leaf node that owns it (assuming the large_buf isn't tied to
// the ref stored directly within the node.  But that would be
// improper).

void co_acquire_large_buf_for_unprepend(const thread_saver_t& saver, large_buf_t *lb, int64_t length);
void co_acquire_large_buf_slice(const thread_saver_t& saver, large_buf_t *lb, int64_t offset, int64_t size, threadsafe_cond_t *acquisition_cond = NULL);
void co_acquire_large_buf(const thread_saver_t& saver, large_buf_t *large_value, threadsafe_cond_t *acquisition_cond = NULL);
void co_acquire_large_buf_lhs(const thread_saver_t& saver, large_buf_t *large_value);
void co_acquire_large_buf_rhs(const thread_saver_t& saver, large_buf_t *large_value);
void co_acquire_large_buf_for_delete(large_buf_t *large_value);

// Avoid using this, use transactor_t instead.
transaction_t *co_begin_transaction(const thread_saver_t& saver, cache_t *cache, access_t access, int expected_change_count, repli_timestamp recency_timestamp);

// Avoid using this, use transactor_t instead.
void co_commit_transaction(const thread_saver_t& saver, transaction_t *transaction);

void co_get_subtree_recencies(transaction_t *transaction, block_id_t *block_ids, size_t num_block_ids, repli_timestamp *recencies_out);


#endif  // __BUFFER_CACHE_CO_FUNCTIONS_HPP__
