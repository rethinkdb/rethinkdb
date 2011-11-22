#ifndef __BTREE_BACKFILL_HPP__
#define __BTREE_BACKFILL_HPP__

#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/fifo_checker.hpp"
#include "memcached/protocol.hpp"
#include "memcached/store.hpp"
#include "utils.hpp"

// Run backfilling at a reduced priority
#define BACKFILL_CACHE_PRIORITY 10

class btree_slice_t;
class btree_key_t;

struct backfill_atom_t {
    store_key_t key;
    boost::intrusive_ptr<data_buffer_t> value;
    mcflags_t flags;
    exptime_t exptime;
    repli_timestamp_t recency;
    cas_t cas_or_zero;
};

// How to use this class: Send on_delete_range calls before
// on_keyvalue calls for keys within that range.
class backfill_callback_t {
public:
    virtual void on_delete_range(const btree_key_t *low, const btree_key_t *high) = 0;
    virtual void on_deletion(const btree_key_t *key, repli_timestamp_t recency) = 0;
    virtual void on_keyvalue(backfill_atom_t atom) = 0;
protected:
    virtual ~backfill_callback_t() { }
};

/* `btree_backfill()` is guaranteed to find all changes whose timestamps are greater than
or equal than `since_when` but which reached the tree before `btree_backfill()` was called.
It may also find changes that happened before `since_when`. */

void btree_backfill(btree_slice_t *slice, repli_timestamp_t since_when, repli_timestamp_t maximum_possible_timestamp, const boost::shared_ptr<cache_account_t>& backfill_account, backfill_callback_t *callback, order_token_t token);



class agnostic_backfill_callback_t {
public:
    virtual void on_delete_range(const btree_key_t *low, const btree_key_t *high) = 0;
    virtual void on_deletion(const btree_key_t *key, repli_timestamp_t recency) = 0;
    virtual void on_pair(transaction_t *txn, repli_timestamp_t recency, const btree_key_t *key, const void *value) = 0;
    virtual ~agnostic_backfill_callback_t() { }
};


void do_agnostic_btree_backfill(value_sizer_t<void> *sizer, btree_slice_t *slice, const key_range_t *key_range, repli_timestamp_t since_when, repli_timestamp_t maximum_possible_timestamp, const boost::shared_ptr<cache_account_t>& backfill_account, agnostic_backfill_callback_t *callback, order_token_t token);


template <class V>
void agnostic_btree_backfill(btree_slice_t *slice, const key_range_t *key_range, repli_timestamp_t since_when, repli_timestamp_t maximum_possible_timestamp, const boost::shared_ptr<cache_account_t>& backfill_account, agnostic_backfill_callback_t *callback, order_token_t token) {
    value_sizer_t<V> sizer(slice->cache()->get_block_size());
    do_agnostic_btree_backfill(&sizer, slice, key_range, since_when, maximum_possible_timestamp, backfill_account, callback, token);
}


#endif  // __BTREE_BACKFILL_HPP__
