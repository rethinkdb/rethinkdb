#include "btree/btree_data_provider.hpp"
#include "buffer_cache/co_functions.hpp"

/* Specialization for small values */

small_value_data_provider_t::small_value_data_provider_t(const btree_value *_value) : value(), buffers() {
    // This can be called in the scheduler thread.

    rassert(!_value->is_large());
    const char *data = ptr_cast<char>(_value->value());
    value.assign(data, data + _value->value_size());
}

size_t small_value_data_provider_t::get_size() const {
    return value.size();
}

const const_buffer_group_t *small_value_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    rassert(!buffers.get());   // This should be the first time this function was called

    buffers.reset(new const_buffer_group_t());
    buffers->add_buffer(get_size(), value.data());
    return buffers.get();
}

/* Specialization for large values */

large_value_data_provider_t::large_value_data_provider_t(const btree_value *value, const boost::shared_ptr<transactor_t>& _transactor) :
    transactor(_transactor),
    lb_ref((*transactor)->cache->get_block_size(), value),
    large_value(transactor, lb_ref.ptr(), btree_value::lbref_limit, rwi_read),
    have_value(false)
{
    // This can be called in the scheduler thread.

    /* We must have gotten into the queue for the first level of the large buf before this
    function returns. The large_buf_t::acquire_slice does this immediately.  This fixes issue #197. */
    large_value.acquire_slice(0, lb_ref.ptr()->size, this);
}

void large_value_data_provider_t::on_large_buf_available(UNUSED large_buf_t *large_buf) {
    rassert(large_value.state == large_buf_t::loaded);
    large_value_cond.pulse();
}

size_t large_value_data_provider_t::get_size() const {
    return lb_ref.ptr()->size;
}

const const_buffer_group_t *large_value_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    rassert(buffers.num_buffers() == 0);   // This should be the first time this function was called

    if (!have_value) {
        large_value_cond.wait();
        have_value = true;
    }
    rassert(large_value.state == large_buf_t::loaded);

    large_value.bufs_at(0, get_size(), true, &buffers);
    return const_view(&buffers);
}

large_value_data_provider_t::~large_value_data_provider_t() {
    /* The buf must be loaded before we are destroyed, or else it will
    try to access us after we are destroyed. */
    if (!have_value) large_value_cond.wait();
}

/* Choose the appropriate specialization */

value_data_provider_t *value_data_provider_t::create(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor) {
    transactor->get()->assert_thread();
    // This can be called in the scheduler thread.
    if (value->is_large()) {
        return new large_value_data_provider_t(value, transactor);
    } else {
        return new small_value_data_provider_t(value);
    }
}


