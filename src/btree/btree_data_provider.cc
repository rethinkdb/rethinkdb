#include "btree/btree_data_provider.hpp"
#include "buffer_cache/co_functions.hpp"

value_data_provider_t::value_data_provider_t(transaction_t *txn, const btree_value_t *value) {
    blob_t blob(const_cast<btree_value_t *>(value)->value_ref(), blob::btree_maxreflen);
    buffer_group_t group;
    blob_acq_t acqs;
    blob.expose_region(txn, rwi_read_outdated_ok, 0, blob.valuesize(), &group, &acqs);
    size_t sz = group.get_size();
    buf.reset(new char[sz]);
    buffers.add_buffer(sz, buf.get());
    buffer_group_copy_data(&buffers, const_view(&group));
}

size_t value_data_provider_t::get_size() const {
    return buffers.get_size();
}

const const_buffer_group_t *value_data_provider_t::get_data_as_buffers() {
    return const_view(&buffers);
}



/* Choose the appropriate specialization */

value_data_provider_t *value_data_provider_t::create(const btree_value_t *value, const boost::shared_ptr<transaction_t>& transaction) {
    transaction->assert_thread();
    return new value_data_provider_t(transaction.get(), value);
}


