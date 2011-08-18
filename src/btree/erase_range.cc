#include "btree/erase_range.hpp"

#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/fifo_checker.hpp"
#include "errors.hpp"


template <class V>
void btree_erase_range_generic(UNUSED value_sizer_t<V> *sizer, UNUSED btree_slice_t *slice,
                               UNUSED key_tester_t *tester,
                               UNUSED const btree_key_t *left_exclusive_or_null,
                               UNUSED const btree_key_t *right_inclusive_or_null,
                               UNUSED order_token_t token) {
    crash("not implemented");



}

void btree_erase_range(btree_slice_t *slice, key_tester_t *tester,
                       bool left_key_supplied, const store_key_t& left_key_exclusive,
                       bool right_key_supplied, const store_key_t& right_key_inclusive,
                       order_token_t token) {
    value_sizer_t<memcached_value_t> mc_sizer(slice->cache()->get_block_size());
    value_sizer_t<void> *sizer = &mc_sizer;

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

    btree_erase_range_generic(sizer, slice, tester, lk, rk, token);
}
