#include "btree/btree_data_provider.hpp"
#include "buffer_cache/co_functions.hpp"

value_data_provider_t::value_data_provider_t(transaction_t *txn, btree_value_t *value)
    : blob(txn->get_cache()->get_block_size(), value->value_ref(), 251) {
    blob.expose_region(txn, rwi_read_outdated_ok, 0, blob.valuesize(), &buffers, &acqs);
}

size_t value_data_provider_t::get_size() const {
    return blob->valuesize();
}

const const_buffer_group_t *value_data_provider_t::get_data_as_buffers() {
    return const_view(&buffers);
}



/* Choose the appropriate specialization */

value_data_provider_t *value_data_provider_t::create(const btree_value *value, const boost::shared_ptr<transaction_t>& transaction) {
    transaction->assert_thread();
    return new value_data_provider_t(transaction.get(), value);
}


