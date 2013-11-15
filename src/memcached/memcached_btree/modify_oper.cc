// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/modify_oper.hpp"

#if SLICE_ALT
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/alt_blob.hpp"
#endif
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/operations.hpp"
#include "btree/slice.hpp"

#if SLICE_ALT
void run_memcached_modify_oper(memcached_modify_oper_t *oper, btree_slice_t *slice,
                               const store_key_t &store_key, cas_t proposed_cas,
                               exptime_t effective_time, repli_timestamp_t timestamp,
                               superblock_t *superblock) {
#else
void run_memcached_modify_oper(memcached_modify_oper_t *oper, btree_slice_t *slice,
                               const store_key_t &store_key, cas_t proposed_cas,
                               exptime_t effective_time, repli_timestamp_t timestamp,
                               transaction_t *txn, superblock_t *superblock) {
#endif

    block_size_t block_size = slice->cache()->get_block_size();

    keyvalue_location_t<memcached_value_t> kv_location;
#if SLICE_ALT
    find_keyvalue_location_for_write(superblock, store_key.btree_key(),
                                     &kv_location, &slice->stats, NULL);
#else
    find_keyvalue_location_for_write(txn, superblock, store_key.btree_key(),
            &kv_location, &slice->root_eviction_priority, &slice->stats, NULL);
#endif
    scoped_malloc_t<memcached_value_t> the_value(std::move(kv_location.value));

    bool expired = the_value.has() && the_value->expired(effective_time);

    // If the value's expired, delete it.
    if (expired) {
#if SLICE_ALT
        rassert(!kv_location.buf.empty());
        alt::blob_t b(slice->cache()->get_block_size(),
                      the_value->value_ref(), alt::blob::btree_maxreflen);
        b.clear(alt::alt_buf_parent_t(&kv_location.buf));
#else
        blob_t b(txn->get_cache()->get_block_size(),
                 the_value->value_ref(), blob::btree_maxreflen);
        b.clear(txn);
#endif
        the_value.reset();
    }

#if SLICE_ALT
    rassert(!kv_location.buf.empty());
    bool update_needed = oper->operate(alt::alt_buf_parent_t(&kv_location.buf),
                                       &the_value);
#else
    bool update_needed = oper->operate(txn, &the_value);
#endif
    update_needed = update_needed || expired;

    // Add a CAS to the value if necessary
    if (the_value.has()) {
        if (the_value->has_cas()) {
            rassert(proposed_cas != BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS);
            the_value->set_cas(block_size, proposed_cas);
        }
    }

    // Actually update the leaf, if needed.
    if (update_needed) {
        kv_location.value = std::move(the_value);
        null_key_modification_callback_t<memcached_value_t> null_cb;
#if SLICE_ALT
        apply_keyvalue_change(&kv_location, store_key.btree_key(), timestamp,
                              expired, &null_cb);
#else
        apply_keyvalue_change(txn, &kv_location, store_key.btree_key(), timestamp, expired, &null_cb, &slice->root_eviction_priority);
#endif
    }
}

