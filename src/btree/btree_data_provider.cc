#include "btree/btree_data_provider.hpp"
#include "buffer_cache/co_functions.hpp"

/* Specialization for small values */

small_value_data_provider_t::small_value_data_provider_t(const btree_value *_value) {
    // This can be called in the scheduler thread.

    rassert(!_value->is_large());
    const char *data = reinterpret_cast<const char *>(_value->value());
    value.assign(data, data + _value->value_size());
    buffer_group.add_buffer(value.size(), value.data());
}

size_t small_value_data_provider_t::get_size() const {
    return value.size();
}

const const_buffer_group_t *small_value_data_provider_t::get_data_as_buffers() {

    return &buffer_group;
}

/* Specialization for large values */

large_value_data_provider_t::large_value_data_provider_t(const btree_value *value, const boost::shared_ptr<transaction_t>& _transaction) :
    transaction(_transaction),
    lb_ref(transaction->cache->get_block_size(), value),
    large_value(transaction, lb_ref.ptr(), btree_value::lbref_limit, rwi_read_outdated_ok)
{
    // This can be called in the scheduler thread.

    /* We must have gotten into the queue for the first level of the large buf before this
    function returns. The large_buf_t::acquire_slice does this immediately.  This fixes issue #197. */
    large_value.acquire_slice(0, lb_ref.ptr()->size, this);
}

void large_value_data_provider_t::on_large_buf_available(UNUSED large_buf_t *large_buf) {
    rassert(large_value.state == large_buf_t::loaded);

    /* Fill `buffers` with a pointer to the beginning of each buffer and the size
    of that buffer. */
    large_value.bufs_at(0, get_size(), true, &buffers);

    large_value_cond.pulse();
}

size_t large_value_data_provider_t::get_size() const {
    return lb_ref.ptr()->size;
}

const const_buffer_group_t *large_value_data_provider_t::get_data_as_buffers() {
    large_value_cond.wait();
    return const_view(&buffers);
}

large_value_data_provider_t::~large_value_data_provider_t() {
    /* The buf must be loaded before we are destroyed, or else it will
    try to access us after we are destroyed. */
    large_value_cond.wait();
}

/* Choose the appropriate specialization */

value_data_provider_t *value_data_provider_t::create(const btree_value *value, const boost::shared_ptr<transaction_t>& transaction) {
    transaction->assert_thread();
    // This can be called in the scheduler thread.
    if (value->is_large()) {
        return new large_value_data_provider_t(value, transaction);
    } else {
        return new small_value_data_provider_t(value);
    }
}


