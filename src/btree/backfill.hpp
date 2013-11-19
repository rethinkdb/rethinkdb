// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BTREE_BACKFILL_HPP_
#define BTREE_BACKFILL_HPP_

#include <map>
#include <string>

#include "buffer_cache/types.hpp"
#include "containers/uuid.hpp"
#include "utils.hpp"

class btree_slice_t;
struct btree_key_t;
struct key_range_t;
class parallel_traversal_progress_t;
class superblock_t;
template <class> class value_sizer_t;
class repli_timestamp_t;
struct secondary_index_t;
class signal_t;


class agnostic_backfill_callback_t {
public:
    virtual void on_delete_range(const key_range_t &range, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_deletion(const btree_key_t *key, repli_timestamp_t recency, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_pair(transaction_t *txn, repli_timestamp_t recency, const btree_key_t *key, const void *value, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual void on_sindexes(const std::map<std::string, secondary_index_t> &sindexes, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
    virtual ~agnostic_backfill_callback_t() { }
};

/* `do_agnostic_btree_backfill()` is guaranteed to find all changes whose
timestamps are greater than or equal than `since_when` but which reached the
tree before `btree_backfill()` was called. It may also find changes that
happened before `since_when`. */

void do_agnostic_btree_backfill(value_sizer_t<void> *sizer,
        btree_slice_t *slice, const key_range_t& key_range, repli_timestamp_t since_when,
        agnostic_backfill_callback_t *callback, transaction_t *txn,
        superblock_t *superblock, buf_lock_t *sindex_block, parallel_traversal_progress_t *p,
        signal_t *interruptor)
THROWS_ONLY(interrupted_exc_t);

#endif  // BTREE_BACKFILL_HPP_
