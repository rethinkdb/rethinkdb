#ifndef __BTREE_RGET_HPP__
#define __BTREE_RGET_HPP__

#include "utils.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/key_value_store.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/transactor.hpp"
#include "buffer_cache/co_functions.hpp"
#include <boost/shared_ptr.hpp>

struct rget_value_provider_t : public auto_copying_data_provider_t {
    static rget_value_provider_t *create_provider(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor);
    virtual ~rget_value_provider_t() { }
protected:
    rget_value_provider_t() { }
};

struct rget_small_value_provider_t : public rget_value_provider_t {
    typedef std::vector<byte> buffer_t;
    buffer_t value;
    boost::scoped_ptr<const_buffer_group_t> buffers;

    size_t get_size() const {
        return value.size();
    }

    const const_buffer_group_t *get_data_as_buffers() throw (data_provider_failed_exc_t) {
        rassert(!buffers.get());

        buffers.reset(new const_buffer_group_t());
        buffers->add_buffer(get_size(), value.data());
        return buffers.get();
    }
protected:
    rget_small_value_provider_t(const btree_value *value) : value(), buffers() {
        rassert(!value->is_large());
        const byte *data = ptr_cast<byte>(value->value());
        this->value.assign(data, data + value->value_size());
    }

    friend class rget_value_provider_t;
};

struct rget_large_value_provider_t : public rget_value_provider_t {
    virtual ~rget_large_value_provider_t();

    size_t get_size() const {
        return lb_ref.size;
    }

    const const_buffer_group_t *get_data_as_buffers() throw (data_provider_failed_exc_t) {
        rassert(!buffers);
        rassert(!large_value);

        large_value.reset(new large_buf_t(transactor->transaction()));
        co_acquire_large_value(large_value.get(), lb_ref, rwi_read);

        rassert(large_value->state == large_buf_t::loaded);
        rassert(large_value->get_root_ref().block_id == lb_ref.block_id);

        buffers.reset(new const_buffer_group_t());
        for (int i = 0; i < large_value->get_num_segments(); i++) {
            uint16_t size;
            const void *data = large_value->get_segment(i, &size);
            buffers->add_buffer(size, data);
        }
        return buffers.get();
    }
private:
    rget_large_value_provider_t(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor)
        : transactor(transactor), buffers(), lb_ref(value->lb_ref()) {
    }

    boost::shared_ptr<transactor_t> transactor;
    boost::scoped_ptr<const_buffer_group_t> buffers;
    boost::scoped_ptr<large_buf_t> large_value; // RSI unique_ptr
    large_buf_ref lb_ref;

    friend class rget_value_provider_t;
};

store_t::rget_result_t btree_rget(btree_key_value_store_t *store, store_key_t *start, store_key_t *end, bool left_open, bool right_open, uint64_t max_results);
store_t::rget_result_t btree_rget_slice(btree_slice_t *slice, store_key_t *start, store_key_t *end, bool left_open, bool right_open, uint64_t max_results);

#endif // __BTREE_RGET_HPP__

