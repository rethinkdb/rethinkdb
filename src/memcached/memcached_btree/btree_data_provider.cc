// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/btree_data_provider.hpp"

#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/alt_blob.hpp"
#include "containers/buffer_group.hpp"
#include "containers/data_buffer.hpp"
#include "memcached/memcached_btree/value.hpp"

using alt::alt_access_t;
using alt::alt_buf_parent_t;

counted_t<data_buffer_t> value_to_data_buffer(const memcached_value_t *value,
                                              alt_buf_parent_t parent) {
    parent.cache()->assert_thread();

    alt::blob_t blob(parent.cache()->get_block_size(),
                     const_cast<memcached_value_t *>(value)->value_ref(),
                     alt::blob::btree_maxreflen);

    buffer_group_t group;
    alt::blob_acq_t acqs;
    // RSI: Before we did rwi_read_outdated_ok -- should we snapshot or something?
    blob.expose_region(parent, alt_access_t::read, 0, blob.valuesize(), &group, &acqs);
    size_t sz = group.get_size();
    counted_t<data_buffer_t> ret = data_buffer_t::create(sz);
    buffer_group_t tmp;
    tmp.add_buffer(sz, ret->buf());
    buffer_group_copy_data(&tmp, const_view(&group));

    return ret;
}


