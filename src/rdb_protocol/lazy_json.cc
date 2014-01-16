// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/lazy_json.hpp"

#include "containers/archive/buffer_group_stream.hpp"
#include "rdb_protocol/blob_wrapper.hpp"

#if SLICE_ALT
using namespace alt;  // RSI
#endif

#if SLICE_ALT
counted_t<const ql::datum_t> get_data(const rdb_value_t *value,
                                      alt_buf_parent_t parent) {
#else
counted_t<const ql::datum_t> get_data(const rdb_value_t *value,
                                      transaction_t *txn) {
#endif
#if SLICE_ALT
    rdb_blob_wrapper_t blob(parent.cache()->get_block_size(),
                            const_cast<rdb_value_t *>(value)->value_ref(),
                            alt::blob::btree_maxreflen);
#else
    rdb_blob_wrapper_t blob(txn->get_cache()->get_block_size(),
                            const_cast<rdb_value_t *>(value)->value_ref(),
                            blob::btree_maxreflen);
#endif

    counted_t<const ql::datum_t> data;

#if SLICE_ALT
    alt::blob_acq_t acq_group;
    buffer_group_t buffer_group;
    blob.expose_all(parent, alt_access_t::read, &buffer_group, &acq_group);
#else
    blob_acq_t acq_group;
    buffer_group_t buffer_group;
    blob.expose_all(txn, rwi_read, &buffer_group, &acq_group);
#endif
    buffer_group_read_stream_t read_stream(const_view(&buffer_group));
    archive_result_t res = deserialize(&read_stream, &data);
    guarantee_deserialization(res, "rdb value");

    return data;
}

const counted_t<const ql::datum_t> &lazy_json_t::get() const {
    guarantee(pointee.has());
    if (!pointee->ptr.has()) {
#if SLICE_ALT
        pointee->ptr = get_data(pointee->rdb_value, pointee->parent);
#else
        pointee->ptr = get_data(pointee->rdb_value, pointee->txn);
#endif
        pointee->rdb_value = NULL;
#if SLICE_ALT
        pointee->parent = alt_buf_parent_t();
#else
        pointee->txn = NULL;
#endif
    }
    return pointee->ptr;
}

void lazy_json_t::reset() {
    pointee.reset();
}
