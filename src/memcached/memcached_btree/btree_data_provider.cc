// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "memcached/memcached_btree/btree_data_provider.hpp"

#include "buffer_cache/blob.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "containers/buffer_group.hpp"
#include "containers/data_buffer.hpp"
#include "memcached/memcached_btree/value.hpp"

intrusive_ptr_t<data_buffer_t> value_to_data_buffer(const memcached_value_t *value, transaction_t *txn) {
    txn->assert_thread();

    blob_t blob(const_cast<memcached_value_t *>(value)->value_ref(), blob::btree_maxreflen);

    buffer_group_t group;
    blob_acq_t acqs;
    blob.expose_region(txn, rwi_read_outdated_ok, 0, blob.valuesize(), &group, &acqs);
    size_t sz = group.get_size();
    intrusive_ptr_t<data_buffer_t> ret = data_buffer_t::create(sz);
    buffer_group_t tmp;
    tmp.add_buffer(sz, ret->buf());
    buffer_group_copy_data(&tmp, const_view(&group));

    return ret;
}


