#ifndef __BTREE_BACKFILL_HPP__
#define __BTREE_BACKFILL_HPP__

#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/fifo_checker.hpp"
#include "memcached/store.hpp"
#include "utils.hpp"

// Run backfilling at a reduced priority
#define BACKFILL_CACHE_PRIORITY 10

class btree_slice_t;
struct btree_key_t;

struct backfill_atom_t {
    store_key_t key;
    boost::intrusive_ptr<data_buffer_t> value;
    mcflags_t flags;
    exptime_t exptime;
    repli_timestamp_t recency;
    cas_t cas_or_zero;

    backfill_atom_t() { }
    backfill_atom_t(const store_key_t& key_, const boost::intrusive_ptr<data_buffer_t>& value_, const mcflags_t& flags_, const exptime_t& exptime_, const repli_timestamp_t& recency_, const cas_t& cas_or_zero_)
        : key(key_), value(value_), flags(flags_), exptime(exptime_), recency(recency_), cas_or_zero(cas_or_zero_) { }
};

// How to use this class: Send on_delete_range calls before
// on_keyvalue calls for keys within that range.
class backfill_callback_t {
public:
    virtual void on_delete_range(const btree_key_t *left_exclusive, const btree_key_t *right_inclusive) = 0;
    virtual void on_deletion(const btree_key_t *key, repli_timestamp_t recency) = 0;
    virtual void on_keyvalue(const backfill_atom_t& atom) = 0;
protected:
    virtual ~backfill_callback_t() { }
};

/* `btree_backfill()` is guaranteed to find all changes whose timestamps are greater than
or equal than `since_when` but which reached the tree before `btree_backfill()` was called.
It may also find changes that happened before `since_when`.
*/

void btree_backfill(btree_slice_t *slice, const key_range_t& key_range, repli_timestamp_t since_when,
                    const boost::shared_ptr<cache_account_t>& backfill_account, backfill_callback_t *callback, order_token_t token);
void btree_backfill(btree_slice_t *slice, const key_range_t& key_range, repli_timestamp_t since_when, backfill_callback_t *callback,
                    transaction_t *txn, got_superblock_t& superblock);


class agnostic_backfill_callback_t {
public:
    virtual void on_delete_range(const btree_key_t *low, const btree_key_t *high) = 0;
    virtual void on_deletion(const btree_key_t *key, repli_timestamp_t recency) = 0;
    virtual void on_pair(transaction_t *txn, repli_timestamp_t recency, const btree_key_t *key, const void *value) = 0;
    virtual ~agnostic_backfill_callback_t() { }
};


void do_agnostic_btree_backfill(value_sizer_t<void> *sizer, btree_slice_t *slice,
    const key_range_t& key_range, repli_timestamp_t since_when, const boost::shared_ptr<cache_account_t>& backfill_account,
    agnostic_backfill_callback_t *callback, order_token_t token);

template <class V>
void agnostic_btree_backfill(btree_slice_t *slice, const key_range_t& key_range, repli_timestamp_t since_when,
                             const boost::shared_ptr<cache_account_t>& backfill_account, agnostic_backfill_callback_t *callback, order_token_t token) {
    value_sizer_t<V> sizer(slice->cache()->get_block_size());
    do_agnostic_btree_backfill(&sizer, slice, key_range, since_when, backfill_account, callback, token);
}


#endif  // __BTREE_BACKFILL_HPP__
