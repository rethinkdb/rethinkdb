#include "rdb_protocol/lazy_json.hpp"

#include "containers/archive/buffer_group_stream.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"

boost::shared_ptr<scoped_cJSON_t> get_data(const rdb_value_t *value,
                                           transaction_t *txn) {
    blob_t blob(txn->get_cache()->get_block_size(),
                const_cast<rdb_value_t *>(value)->value_ref(), blob::btree_maxreflen);

    boost::shared_ptr<scoped_cJSON_t> data;

    blob_acq_t acq_group;
    buffer_group_t buffer_group;
    blob.expose_all(txn, rwi_read, &buffer_group, &acq_group);
    buffer_group_read_stream_t read_stream(const_view(&buffer_group));
    int res = deserialize(&read_stream, &data);
    guarantee_err(res == 0, "disk corruption (or programmer error) detected");

    return data;
}

const boost::shared_ptr<scoped_cJSON_t> &lazy_json_t::get() const {
    if (!pointee->ptr) {
        pointee->ptr = get_data(pointee->rdb_value, pointee->txn);
    }
    return pointee->ptr;
}
