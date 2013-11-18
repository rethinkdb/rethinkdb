// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/erase_range.hpp"

#include "btree/slice.hpp"
#if SLICE_ALT
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/alt_blob.hpp"
#endif
#include "buffer_cache/buffer_cache.hpp"
#include "memcached/memcached_btree/node.hpp"
#include "memcached/memcached_btree/value.hpp"

#if SLICE_ALT
using namespace alt;  // RSI
#endif

#if SLICE_ALT
void memcached_erase_range(btree_slice_t *slice, key_tester_t *tester,
                           bool left_key_supplied,
                           const store_key_t& left_key_exclusive,
                           bool right_key_supplied,
                           const store_key_t& right_key_inclusive,
                           superblock_t *superblock, signal_t *interruptor) {
#else
void memcached_erase_range(btree_slice_t *slice, key_tester_t *tester,
                       bool left_key_supplied, const store_key_t& left_key_exclusive,
                       bool right_key_supplied, const store_key_t& right_key_inclusive,
                       transaction_t *txn, superblock_t *superblock, signal_t *interruptor) {
#endif

    value_sizer_t<memcached_value_t> mc_sizer(slice->cache()->get_block_size());
    value_sizer_t<void> *sizer = &mc_sizer;

    struct : public value_deleter_t {
#if SLICE_ALT
        void delete_value(alt_buf_parent_t leaf_node, void *value) {
#else
        void delete_value(transaction_t *_txn, void *value) {
#endif
#if SLICE_ALT
            alt::blob_t blob(leaf_node.cache()->get_block_size(),
                             static_cast<memcached_value_t *>(value)->value_ref(),
                             alt::blob::btree_maxreflen);
            blob.clear(leaf_node);
#else
            blob_t blob(_txn->get_cache()->get_block_size(),
                        static_cast<memcached_value_t *>(value)->value_ref(),
                        blob::btree_maxreflen);
            blob.clear(_txn);
#endif
        }
    } deleter;

#if SLICE_ALT
    btree_erase_range_generic(sizer, slice, tester, &deleter,
        left_key_supplied ? left_key_exclusive.btree_key() : NULL,
        right_key_supplied ? right_key_inclusive.btree_key() : NULL,
        superblock, interruptor);
#else
    btree_erase_range_generic(sizer, slice, tester, &deleter,
        left_key_supplied ? left_key_exclusive.btree_key() : NULL,
        right_key_supplied ? right_key_inclusive.btree_key() : NULL,
        txn, superblock, interruptor);
#endif
}

#if SLICE_ALT
void memcached_erase_range(btree_slice_t *slice, key_tester_t *tester,
                           const key_range_t &keys,
                           superblock_t *superblock, signal_t *interruptor) {
#else
void memcached_erase_range(btree_slice_t *slice, key_tester_t *tester,
                       const key_range_t &keys,
                       transaction_t *txn, superblock_t *superblock, signal_t *interruptor) {
#endif
    /* This is guaranteed because the way the keys are calculated below would
     * lead to a single key being deleted even if the range was empty. */
    guarantee(!keys.is_empty());
    store_key_t left_exclusive(keys.left);
    store_key_t right_inclusive(keys.right.key);

    bool left_key_supplied = left_exclusive.decrement();
    bool right_key_supplied = !keys.right.unbounded;
    if (right_key_supplied) {
        right_inclusive.decrement();
    }
#if SLICE_ALT
    memcached_erase_range(slice, tester, left_key_supplied, left_exclusive,
                          right_key_supplied, right_inclusive, superblock,
                          interruptor);
#else
    memcached_erase_range(slice, tester, left_key_supplied, left_exclusive, right_key_supplied, right_inclusive, txn, superblock, interruptor);
#endif
}

