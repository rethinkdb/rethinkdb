// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/erase_range.hpp"

#include "btree/btree_store.hpp"
#include "btree/operations.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/blob.hpp"
#include "memcached/memcached_btree/node.hpp"
#include "memcached/memcached_btree/value.hpp"

void memcached_erase_range(key_tester_t *tester,
                           bool left_key_supplied,
                           const store_key_t& left_key_exclusive,
                           bool right_key_supplied,
                           const store_key_t& right_key_inclusive,
                           superblock_t *superblock, signal_t *interruptor) {
    value_sizer_t<memcached_value_t> mc_sizer(superblock->cache()->get_block_size());
    value_sizer_t<void> *sizer = &mc_sizer;

    struct : public value_deleter_t {
        value_sizer_t<memcached_value_t> *sizer_;
        void delete_value(buf_parent_t leaf_node, const void *value) const {
            // To not destroy constness, we operate on a copy of the value
            scoped_malloc_t<memcached_value_t> value_copy(sizer_->max_possible_size());
            memcpy(value_copy.get(), value, sizer_->size(value));
            blob_t blob(leaf_node.cache()->get_block_size(),
                        value_copy->value_ref(),
                        blob::btree_maxreflen);
            blob.clear(leaf_node);
        }
    } deleter;
    deleter.sizer_ = &mc_sizer;

    btree_erase_range_generic(sizer, tester, &deleter,
        left_key_supplied ? left_key_exclusive.btree_key() : NULL,
        right_key_supplied ? right_key_inclusive.btree_key() : NULL,
        superblock, interruptor);
}

void memcached_erase_range(key_tester_t *tester,
                           const key_range_t &keys,
                           superblock_t *superblock, signal_t *interruptor) {
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
    memcached_erase_range(tester, left_key_supplied, left_exclusive,
                          right_key_supplied, right_inclusive, superblock,
                          interruptor);
}

