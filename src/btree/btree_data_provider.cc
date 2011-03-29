#include "btree/btree_data_provider.hpp"
#include "buffer_cache/co_functions.hpp"

/* Specialization for small values */

small_value_data_provider_t::small_value_data_provider_t(const btree_value *_value) : value(), buffers() {
    rassert(!_value->is_large());
    const byte *data = ptr_cast<byte>(_value->value());
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
    large_value(transactor, lb_ref.ptr(), btree_value::lbref_limit, rwi_read)
{
    /* We must have gotten into the queue for the first level of the large buf before this
    function returns. acquire_in_background() will cause acquisition_cond to be pulsed when
    it has started acquiring the large buf. This fixes issue #197. */
    threadsafe_cond_t acquisition_cond;
    coro_t::spawn_now(boost::bind(&large_value_data_provider_t::acquire_in_background, this, &acquisition_cond));
    acquisition_cond.wait();
}

size_t large_value_data_provider_t::get_size() const {
    return lb_ref.ptr()->size;
}

void large_value_data_provider_t::acquire_in_background(threadsafe_cond_t *acquisition_cond) {
    {
        thread_saver_t ts;
        co_acquire_large_buf(ts, &large_value, acquisition_cond);
    }
    rassert(large_value.state == large_buf_t::loaded);
    large_value_cond.pulse();
}

const const_buffer_group_t *large_value_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    rassert(buffers.num_buffers() == 0);   // This should be the first time this function was called

    large_value_cond.wait();
    rassert(large_value.state == large_buf_t::loaded);

    large_value.bufs_at(0, get_size(), true, &buffers);
    return const_view(&buffers);
}

/* Choose the appropriate specialization */

value_data_provider_t *value_data_provider_t::create(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor) {
    if (value->is_large()) {
        return new large_value_data_provider_t(value, transactor);
    } else {
        return new small_value_data_provider_t(value);
    }
}


