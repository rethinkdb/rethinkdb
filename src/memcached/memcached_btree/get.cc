// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/get.hpp"

#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/operations.hpp"
#include "memcached/memcached_btree/btree_data_provider.hpp"
#include "memcached/memcached_btree/node.hpp"
#include "memcached/memcached_btree/value.hpp"

#if SLICE_ALT
get_result_t memcached_get(const store_key_t &store_key,
                           btree_slice_t *slice, exptime_t effective_time,
                           superblock_t *superblock) {
#else
get_result_t memcached_get(const store_key_t &store_key, btree_slice_t *slice, exptime_t effective_time, transaction_t *txn, superblock_t *superblock) {
#endif

    keyvalue_location_t<memcached_value_t> kv_location;
#if SLICE_ALT
    find_keyvalue_location_for_read(superblock, store_key.btree_key(),
                                    &kv_location, &slice->stats, NULL);
#else
    find_keyvalue_location_for_read(txn, superblock, store_key.btree_key(), &kv_location, slice->root_eviction_priority, &slice->stats, NULL);
#endif

    if (!kv_location.value.has()) {
        return get_result_t();
    }

    const memcached_value_t *value = kv_location.value.get();
    if (value->expired(effective_time)) {
        // TODO signal the parser to start deleting the key in the background
        return get_result_t();
    }

    // RSI: We could make this more efficient -- by releasing the leaf node while we
    // acquire blobs as children of this.
#if SLICE_ALT
    counted_t<data_buffer_t> dp
        = value_to_data_buffer(value, buf_parent_t(&kv_location.buf));
#else
    counted_t<data_buffer_t> dp = value_to_data_buffer(value, txn);
#endif

    return get_result_t(dp, value->mcflags(), 0);
}

