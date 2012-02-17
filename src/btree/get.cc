#include "btree/get.hpp"

#include "btree/delete_expired.hpp"
#include "btree/btree_data_provider.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/operations.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "memcached/store.hpp"

get_result_t btree_get(const store_key_t &store_key, btree_slice_t *slice, sequence_group_t *seq_group, order_token_t token) {
    slice->assert_thread();

    boost::scoped_ptr<transaction_t> txn;
    got_superblock_t superblock;
    get_btree_superblock_for_reading(slice, seq_group, rwi_read, token, false, &superblock, txn);
    return btree_get(store_key, slice, txn.get(), superblock);
}

get_result_t btree_get(const store_key_t &store_key, btree_slice_t *slice, transaction_t *txn, got_superblock_t& superblock) {

    btree_key_buffer_t kbuffer(store_key);
    btree_key_t *key = kbuffer.key();

    keyvalue_location_t<memcached_value_t> kv_location;
    find_keyvalue_location_for_read(txn, &superblock, key, &kv_location, slice->root_eviction_priority);

    if (!kv_location.value) {
        return get_result_t();
    }

    const memcached_value_t *value = kv_location.value.get();
    if (value->expired()) {
        // If the value is expired, delete it in the background.
        btree_delete_expired(store_key, slice);
        return get_result_t();
    }

    boost::intrusive_ptr<data_buffer_t> dp = value_to_data_buffer(value, txn);

    return get_result_t(dp, value->mcflags(), 0);
}

