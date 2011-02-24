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

large_value_data_provider_t::large_value_data_provider_t(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor)
    : transactor(transactor), buffers() {
    memcpy(&lb_ref, value->lb_ref(), LARGE_BUF_REF_SIZE);
}

size_t large_value_data_provider_t::get_size() const {
    return lb_ref.size;
}

const const_buffer_group_t *large_value_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    rassert(!buffers);
    rassert(!large_value);

    large_value.reset(new large_buf_t(transactor->transaction()));
    co_acquire_large_value(large_value.get(), &lb_ref, rwi_read);

    rassert(large_value->state == large_buf_t::loaded);

    buffers.reset(new const_buffer_group_t());
    for (int i = 0; i < large_value->get_num_segments(); i++) {
        uint16_t size;
        const void *data = large_value->get_segment(i, &size);
        buffers->add_buffer(size, data);
    }
    return buffers.get();
}

large_value_data_provider_t::~large_value_data_provider_t() {
    if (large_value) {
        large_value->release();
    }
}

value_data_provider_t *value_data_provider_t::create(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor) {
    if (value->is_large())
        return new large_value_data_provider_t(value, transactor);
    else
        return new small_value_data_provider_t(value);
}


