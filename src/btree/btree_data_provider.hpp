#ifndef __BTREE_BTREE_DATA_PROVIDER_HPP__
#define __BTREE_BTREE_DATA_PROVIDER_HPP__

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "btree/value.hpp"
#include "buffer_cache/transactor.hpp"
#include "data_provider.hpp"

class value_data_provider_t : public auto_copying_data_provider_t {
protected:
    value_data_provider_t() { }
public:
    virtual ~value_data_provider_t() { }

    /* When create() returns, it is safe for the caller to invalidate "value" and it is
    safe for the caller to release the leaf node that "value" came from. */
    // TODO: Make this return a unique_ptr_t.
    static value_data_provider_t *create(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor);
};

class small_value_data_provider_t : public value_data_provider_t {
private:
    small_value_data_provider_t(const btree_value *value);

public:
    size_t get_size() const;
    const const_buffer_group_t *get_data_as_buffers() throw (data_provider_failed_exc_t);

private:
    // TODO: just use char[MAX_IN_NODE_VALUE_SIZE], thanks.
    typedef std::vector<char> buffer_t;
    buffer_t value;
    boost::scoped_ptr<const_buffer_group_t> buffers;

    friend class value_data_provider_t;
};

class large_value_data_provider_t : public value_data_provider_t, public large_buf_available_callback_t {
private:
    large_value_data_provider_t(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor);

public:
    size_t get_size() const;
    const const_buffer_group_t *get_data_as_buffers() throw (data_provider_failed_exc_t);
    ~large_value_data_provider_t();

private:
    /* We hold a shared pointer to the transactor so the transaction does not end while
    we still hold the large value. */
    boost::shared_ptr<transactor_t> transactor;

    buffer_group_t buffers;

    struct large_buf_ref_buffer_t {
        large_buf_ref_buffer_t(block_size_t block_size, const btree_value *value) {
            memcpy(ptr(), value->lb_ref(), value->lb_ref()->refsize(block_size, btree_value::lbref_limit));
        }
        const large_buf_ref *ptr() const {
            return reinterpret_cast<const large_buf_ref *>(buffer);
        }
        large_buf_ref *ptr() {
            return reinterpret_cast<large_buf_ref *>(buffer);
        }
        char buffer[MAX_IN_NODE_VALUE_SIZE];
    };

    large_buf_ref_buffer_t lb_ref;
    large_buf_t large_value;
    cond_t large_value_cond;
    bool have_value;
    void on_large_buf_available(large_buf_t *large_buf);

    friend class value_data_provider_t;
};

#endif // __BTREE_BTREE_DATA_PROVIDER_HPP__
