#include "btree/get.hpp"

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "btree/delete_expired.hpp"
#include "btree/btree_data_provider.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/operations.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "store.hpp"

get_result_t btree_get(const store_key_t &store_key, btree_slice_t *slice, order_token_t token) {
    btree_key_buffer_t kbuffer(store_key);
    btree_key_t *key = kbuffer.key();

    slice->assert_thread();
    on_thread_t mover(slice->home_thread());

    got_superblock_t got;
    get_btree_superblock(slice, rwi_read, token, &got);

    memcached_value_sizer_t sizer(slice->cache()->get_block_size());
    keyvalue_location_t kv_location;
    find_keyvalue_location_for_read(&sizer, &got, key, &kv_location);

    if (!kv_location.value) {
        return get_result_t();
    }

    const btree_value_t *value = reinterpret_cast<const btree_value_t *>(kv_location.value.get());
    if (value->expired()) {
        // If the value is expired, delete it in the background.
        btree_delete_expired(store_key, slice);
        return get_result_t();
    }

    boost::shared_ptr<value_data_provider_t> dp(value_data_provider_t::create(value, kv_location.txn.get()));

    return get_result_t(dp, value->mcflags(), 0);
}
