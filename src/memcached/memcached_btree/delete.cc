// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/delete.hpp"

#if SLICE_ALT
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/alt_blob.hpp"
#endif
#include "memcached/memcached_btree/modify_oper.hpp"
#include "repli_timestamp.hpp"

struct memcached_delete_oper_t : public memcached_modify_oper_t {

    memcached_delete_oper_t(bool dont_record, btree_slice_t *_slice) : dont_put_in_delete_queue(dont_record), slice(_slice) { }

    delete_result_t result;
    bool dont_put_in_delete_queue;
    btree_slice_t *slice;

#if SLICE_ALT
    bool operate(alt::alt_buf_parent_t leaf,
                 scoped_malloc_t<memcached_value_t> *value) {
#else
    bool operate(transaction_t *txn, scoped_malloc_t<memcached_value_t> *value) {
#endif
        if (value->has()) {
            result = dr_deleted;
            {
#if SLICE_ALT
                alt::blob_t b(leaf.cache()->get_block_size(),
                              (*value)->value_ref(), alt::blob::btree_maxreflen);
                b.clear(leaf);
#else
                blob_t b(txn->get_cache()->get_block_size(),
                         (*value)->value_ref(), blob::btree_maxreflen);
                b.clear(txn);
#endif
            }
            value->reset();
            return true;
        } else {
            result = dr_not_found;
            return false;
        }
    }

    int compute_expected_change_count(UNUSED block_size_t block_size) {
        return 1;
    }
};

#if SLICE_ALT
delete_result_t
memcached_delete(const store_key_t &key, bool dont_put_in_delete_queue,
                 btree_slice_t *slice, exptime_t effective_time,
                 repli_timestamp_t timestamp, superblock_t *superblock) {
#else
delete_result_t memcached_delete(const store_key_t &key, bool dont_put_in_delete_queue, btree_slice_t *slice, exptime_t effective_time, repli_timestamp_t timestamp,
    transaction_t *txn, superblock_t *superblock) {
#endif

    memcached_delete_oper_t oper(dont_put_in_delete_queue, slice);
#if SLICE_ALT
    run_memcached_modify_oper(&oper, slice, key, BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS, effective_time, timestamp, superblock);
#else
    run_memcached_modify_oper(&oper, slice, key, BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS, effective_time, timestamp, txn, superblock);
#endif
    return oper.result;
}

