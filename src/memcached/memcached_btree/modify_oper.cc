// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/modify_oper.hpp"

#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/blob.hpp"
#include "btree/btree_store.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/operations.hpp"
#include "btree/slice.hpp"

void run_memcached_modify_oper(memcached_modify_oper_t *oper, btree_slice_t *slice,
                               const store_key_t &store_key, cas_t proposed_cas,
                               exptime_t effective_time, repli_timestamp_t timestamp,
                               superblock_t *superblock,
                               promise_t<superblock_t *> *pass_back_superblock) {

    block_size_t block_size = slice->cache()->get_block_size();

    keyvalue_location_t<memcached_value_t> kv_location;
    noop_value_deleter_t no_detacher;
    find_keyvalue_location_for_write(superblock, store_key.btree_key(), &no_detacher,
                                     &kv_location, &slice->stats,
                                     static_cast<profile::trace_t *>(NULL),
                                     pass_back_superblock);
    scoped_malloc_t<memcached_value_t> the_value(std::move(kv_location.value));

    bool expired = the_value.has() && the_value->expired(effective_time);

    // If the value's expired, delete it.
    if (expired) {
        rassert(!kv_location.buf.empty());
        blob_t b(slice->cache()->get_block_size(),
                      the_value->value_ref(), blob::btree_maxreflen);
        b.clear(buf_parent_t(&kv_location.buf));
        the_value.reset();
    }

    rassert(!kv_location.buf.empty());
    bool update_needed = oper->operate(buf_parent_t(&kv_location.buf),
                                       &the_value);
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
        apply_keyvalue_change(&kv_location, store_key.btree_key(), timestamp,
                              expired ? expired_t::YES : expired_t::NO, &no_detacher,
                              &null_cb);
    }
}

