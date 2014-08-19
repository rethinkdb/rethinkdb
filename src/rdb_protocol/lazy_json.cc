// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/lazy_json.hpp"

#include "containers/archive/buffer_group_stream.hpp"
#include "containers/archive/versioned.hpp"
#include "rdb_protocol/blob_wrapper.hpp"

ql::datum_t get_data(const rdb_value_t *value, buf_parent_t parent) {
    // TODO: Just use deserialize_from_blob?
    rdb_blob_wrapper_t blob(parent.cache()->max_block_size(),
                            const_cast<rdb_value_t *>(value)->value_ref(),
                            blob::btree_maxreflen);

    ql::datum_t data;

    blob_acq_t acq_group;
    buffer_group_t buffer_group;
    blob.expose_all(parent, access_t::read, &buffer_group, &acq_group);
    buffer_group_read_stream_t read_stream(const_view(&buffer_group));
    archive_result_t res
        = datum_deserialize(&read_stream, &data);
    guarantee_deserialization(res, "rdb value");

    return data;
}

const ql::datum_t &lazy_json_t::get() const {
    guarantee(pointee.has());
    if (!pointee->ptr.has()) {
        pointee->ptr = get_data(pointee->rdb_value, pointee->parent);
        pointee->rdb_value = NULL;
        pointee->parent = buf_parent_t();
    }
    return pointee->ptr;
}

bool lazy_json_t::references_parent() const {
    return pointee.has() && !pointee->parent.empty();
}

void lazy_json_t::reset() {
    pointee.reset();
}
