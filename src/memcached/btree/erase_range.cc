#include "memcached/btree/erase_range.hpp"

#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "memcached/btree/node.hpp"
#include "memcached/btree/value.hpp"

void memcached_erase_range(btree_slice_t *slice, key_tester_t *tester,
                       bool left_key_supplied, const store_key_t& left_key_exclusive,
                       bool right_key_supplied, const store_key_t& right_key_inclusive,
                       transaction_t *txn, superblock_t *superblock) {

    value_sizer_t<memcached_value_t> mc_sizer(slice->cache()->get_block_size());
    value_sizer_t<void> *sizer = &mc_sizer;

    struct : public value_deleter_t {
        void delete_value(transaction_t *txn, void *value) {
            blob_t blob(static_cast<memcached_value_t *>(value)->value_ref(), blob::btree_maxreflen);
            blob.clear(txn);
        }
    } deleter;

    // TODO: Sigh, stupid wasteful copies.
    btree_key_buffer_t left, right;
    btree_key_t *lk = NULL, *rk = NULL;
    if (left_key_supplied) {
        left.assign(left_key_exclusive.contents, left_key_exclusive.contents + left_key_exclusive.size);
        lk = left.key();
    }
    if (right_key_supplied) {
        const char *p = right_key_inclusive.contents;
        right.assign(p, p + right_key_inclusive.size);
        rk = right.key();
    }

    btree_erase_range_generic(sizer, slice, tester, &deleter, lk, rk, txn, superblock);
}

void memcached_erase_range(btree_slice_t *slice, key_tester_t *tester,
                       const key_range_t &keys,
                       transaction_t *txn, superblock_t *superblock) {
    store_key_t left_exclusive(keys.left);
    store_key_t right_inclusive(keys.right.key);

    bool left_key_supplied = left_exclusive.decrement();
    bool right_key_supplied = !keys.right.unbounded;
    if (right_key_supplied) {
        right_inclusive.decrement();
    }
    memcached_erase_range(slice, tester, left_key_supplied, left_exclusive, right_key_supplied, right_inclusive, txn, superblock);
}

