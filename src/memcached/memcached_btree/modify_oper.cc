#include "memcached/memcached_btree/modify_oper.hpp"

#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/operations.hpp"
#include "btree/slice.hpp"

void run_memcached_modify_oper(memcached_modify_oper_t *oper, btree_slice_t *slice, const store_key_t &store_key, cas_t proposed_cas, exptime_t effective_time, repli_timestamp_t timestamp,
    transaction_t *txn, superblock_t *superblock) {

    block_size_t block_size = slice->cache()->get_block_size();

    keyvalue_location_t<memcached_value_t> kv_location;
    find_keyvalue_location_for_write(txn, superblock, store_key.btree_key(), &kv_location, &slice->root_eviction_priority, &slice->stats);
    scoped_malloc_t<memcached_value_t> the_value;
    the_value.reinterpret_swap(kv_location.value);

    bool expired = the_value.has() && the_value->expired(effective_time);

    // If the value's expired, delete it.
    if (expired) {
        blob_t b(the_value->value_ref(), blob::btree_maxreflen);
        b.clear(txn);
        the_value.reset();
    }

    bool update_needed = oper->operate(txn, &the_value);
    update_needed = update_needed || expired;

    // Add a CAS to the value if necessary
    if (the_value.has()) {
        if (the_value->has_cas()) {
            rassert_unreviewed(proposed_cas != BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS);
            the_value->set_cas(block_size, proposed_cas);
        }
    }

    // Actually update the leaf, if needed.
    if (update_needed) {
        kv_location.value.reinterpret_swap(the_value);
        null_key_modification_callback_t<memcached_value_t> null_cb;
        apply_keyvalue_change(txn, &kv_location, store_key.btree_key(), timestamp, expired, &null_cb, &slice->root_eviction_priority);
    }
}

