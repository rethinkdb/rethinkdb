// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/append_prepend.hpp"

#if SLICE_ALT
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/alt_blob.hpp"
#endif
#include "memcached/memcached_btree/modify_oper.hpp"
#include "containers/buffer_group.hpp"
#include "repli_timestamp.hpp"

struct memcached_append_prepend_oper_t : public memcached_modify_oper_t {

    memcached_append_prepend_oper_t(counted_t<data_buffer_t> _data, bool _append)
        : data(_data), append(_append)
    { }

#if SLICE_ALT
    bool operate(alt::alt_buf_parent_t leaf,
                 scoped_malloc_t<memcached_value_t> *value) {
#else
    bool operate(transaction_t *txn, scoped_malloc_t<memcached_value_t> *value) {
#endif
        if (!value->has()) {
            result = apr_not_found;
            return false;
        }

        size_t new_size = (*value)->value_size() + data->size();
        if (new_size > MAX_VALUE_SIZE) {
            result = apr_too_large;
            return false;
        }

#if SLICE_ALT
        alt::blob_t b(leaf.cache()->get_block_size(),
                      (*value)->value_ref(), alt::blob::btree_maxreflen);
#else
        blob_t b(txn->get_cache()->get_block_size(),
                 (*value)->value_ref(), blob::btree_maxreflen);
#endif
        buffer_group_t buffer_group;
#if SLICE_ALT
        alt::blob_acq_t acqs;
#else
        blob_acq_t acqs;
#endif

        size_t old_size = b.valuesize();
#if SLICE_ALT
        if (append) {
            b.append_region(leaf, data->size());
            b.expose_region(leaf, alt::alt_access_t::write,
                            old_size, data->size(), &buffer_group, &acqs);
        } else {
            b.prepend_region(leaf, data->size());
            b.expose_region(leaf, alt::alt_access_t::write,
                            0, data->size(), &buffer_group, &acqs);
        }
#else
        if (append) {
            b.append_region(txn, data->size());
            b.expose_region(txn, rwi_write, old_size, data->size(), &buffer_group, &acqs);
        } else {
            b.prepend_region(txn, data->size());
            b.expose_region(txn, rwi_write, 0, data->size(), &buffer_group, &acqs);
        }
#endif

        buffer_group_copy_data(&buffer_group, data->buf(), data->size());
        result = apr_success;
        return true;
    }

    int compute_expected_change_count(block_size_t block_size) {
        if (data->size() < MAX_IN_NODE_VALUE_SIZE) {
            return 1;
        } else {
            size_t size = ceil_aligned(data->size(), block_size.value());
            // one for the leaf node plus the number of blocks required to hold the large value
            return 1 + size / block_size.value();
        }
    }

    append_prepend_result_t result;

    counted_t<data_buffer_t> data;
    bool append;   // true = append, false = prepend
};

#if SLICE_ALT
append_prepend_result_t
memcached_append_prepend(const store_key_t &key, btree_slice_t *slice,
                         const counted_t<data_buffer_t>& data, bool append,
                         cas_t proposed_cas, exptime_t effective_time,
                         repli_timestamp_t timestamp, superblock_t *superblock) {
#else
append_prepend_result_t memcached_append_prepend(const store_key_t &key, btree_slice_t *slice, const counted_t<data_buffer_t>& data, bool append, cas_t proposed_cas, exptime_t effective_time, repli_timestamp_t timestamp, transaction_t *txn, superblock_t *superblock) {
#endif
    memcached_append_prepend_oper_t oper(data, append);
#if SLICE_ALT
    run_memcached_modify_oper(&oper, slice, key, proposed_cas,
                              effective_time, timestamp, superblock);
#else
    run_memcached_modify_oper(&oper, slice, key, proposed_cas, effective_time, timestamp, txn, superblock);
#endif
    return oper.result;
}

