#include "btree/btree_data_provider.hpp"
#include "buffer_cache/co_functions.hpp"

small_value_data_provider_t::small_value_data_provider_t(const btree_value *value) : value(), buffers() {
    rassert(!value->is_large());
    const byte *data = ptr_cast<byte>(value->value());
    this->value.assign(data, data + value->value_size());
}

size_t small_value_data_provider_t::get_size() const {
    return value.size();
}

const const_buffer_group_t *small_value_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    rassert(!buffers.get());

    buffers.reset(new const_buffer_group_t());
    buffers->add_buffer(get_size(), value.data());
    return buffers.get();
}

large_value_data_provider_t::large_value_data_provider_t(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor_)
    : transactor(transactor_), buffers() {
    memcpy(&lb_ref, value->lb_ref(), value->lb_ref()->refsize(transactor->transaction()->cache->get_block_size(), btree_value::lbref_limit));
}

size_t large_value_data_provider_t::get_size() const {
    return lb_ref.size;
}

const const_buffer_group_t *large_value_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    rassert(buffers.num_buffers() == 0);
    rassert(!large_value);

    large_value.reset(new large_buf_t(transactor->transaction(), &lb_ref, btree_value::lbref_limit, rwi_read));
    co_acquire_large_buf(large_value.get());

    rassert(large_value->state == large_buf_t::loaded);

    large_value->bufs_at(0, lb_ref.size, true, &buffers);
    return const_view(&buffers);
}

value_data_provider_t *value_data_provider_t::create(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor) {
    if (value->is_large())
        return new large_value_data_provider_t(value, transactor);
    else
        return new small_value_data_provider_t(value);
}


