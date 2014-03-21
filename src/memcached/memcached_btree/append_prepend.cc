// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/append_prepend.hpp"

#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/blob.hpp"
#include "math.hpp"
#include "memcached/memcached_btree/modify_oper.hpp"
#include "containers/buffer_group.hpp"
#include "repli_timestamp.hpp"

struct memcached_append_prepend_oper_t : public memcached_modify_oper_t {

    memcached_append_prepend_oper_t(counted_t<data_buffer_t> _data, bool _append)
        : data(_data), append(_append)
    { }

    bool operate(buf_parent_t leaf,
                 scoped_malloc_t<memcached_value_t> *value) {
        if (!value->has()) {
            result = apr_not_found;
            return false;
        }

        size_t new_size = (*value)->value_size() + data->size();
        if (new_size > MAX_VALUE_SIZE) {
            result = apr_too_large;
            return false;
        }

        blob_t b(leaf.cache()->get_block_size(),
                      (*value)->value_ref(), blob::btree_maxreflen);
        buffer_group_t buffer_group;
        blob_acq_t acqs;

        size_t old_size = b.valuesize();
        if (append) {
            b.append_region(leaf, data->size());
            b.expose_region(leaf, access_t::write,
                            old_size, data->size(), &buffer_group, &acqs);
        } else {
            b.prepend_region(leaf, data->size());
            b.expose_region(leaf, access_t::write,
                            0, data->size(), &buffer_group, &acqs);
        }

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

append_prepend_result_t
memcached_append_prepend(const store_key_t &key, btree_slice_t *slice,
                         const counted_t<data_buffer_t>& data, bool append,
                         cas_t proposed_cas, exptime_t effective_time,
                         repli_timestamp_t timestamp, superblock_t *superblock) {
    memcached_append_prepend_oper_t oper(data, append);
    run_memcached_modify_oper(&oper, slice, key, proposed_cas,
                              effective_time, timestamp, superblock);
    return oper.result;
}

