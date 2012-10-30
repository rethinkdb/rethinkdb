// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/get.hpp"

#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/operations.hpp"
#include "memcached/memcached_btree/btree_data_provider.hpp"
#include "memcached/memcached_btree/node.hpp"
#include "memcached/memcached_btree/value.hpp"

get_result_t memcached_get(const store_key_t &store_key, btree_slice_t *slice, exptime_t effective_time, transaction_t *txn, superblock_t *superblock) {

    keyvalue_location_t<memcached_value_t> kv_location;
    find_keyvalue_location_for_read(txn, superblock, store_key.btree_key(), &kv_location, slice->root_eviction_priority, &slice->stats);

    if (!kv_location.value.has()) {
        return get_result_t();
    }

    const memcached_value_t *value = kv_location.value.get();
    if (value->expired(effective_time)) {
        // TODO signal the parser to start deleting the key in the background
        return get_result_t();
    }

    intrusive_ptr_t<data_buffer_t> dp = value_to_data_buffer(value, txn);

    return get_result_t(dp, value->mcflags(), 0);
}

