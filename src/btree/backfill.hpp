#ifndef BTREE_BACKFILL_HPP_
#define BTREE_BACKFILL_HPP_

#include "btree/node.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/fifo_checker.hpp"
#include "containers/data_buffer.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

class btree_slice_t;
struct btree_key_t;

class agnostic_backfill_callback_t {
public:
    virtual void on_delete_range(const key_range_t &range) = 0;
    virtual void on_deletion(const btree_key_t *key, repli_timestamp_t recency) = 0;
    virtual void on_pair(transaction_t *txn, repli_timestamp_t recency, const btree_key_t *key, const void *value) = 0;
    virtual ~agnostic_backfill_callback_t() { }
};

/* `do_agnostic_btree_backfill()` is guaranteed to find all changes whose
timestamps are greater than or equal than `since_when` but which reached the
tree before `btree_backfill()` was called. It may also find changes that
happened before `since_when`. */

class parallel_traversal_progress_t;
class superblock_t;

void do_agnostic_btree_backfill(value_sizer_t<void> *sizer, btree_slice_t *slice, const key_range_t& key_range, repli_timestamp_t since_when,
                                agnostic_backfill_callback_t *callback, transaction_t *txn, superblock_t *superblock, parallel_traversal_progress_t *);

#endif  // BTREE_BACKFILL_HPP_
