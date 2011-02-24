#ifndef __BTREE_BTREE_DATA_PROVIDER_HPP__
#define __BTREE_BTREE_DATA_PROVIDER_HPP__

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

    static value_data_provider_t *create(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor);
};

class small_value_data_provider_t : public value_data_provider_t {
private:
    small_value_data_provider_t(const btree_value *value);

public:
    size_t get_size() const;
    const const_buffer_group_t *get_data_as_buffers() throw (data_provider_failed_exc_t);

private:
    typedef std::vector<byte> buffer_t;
    buffer_t value;
    boost::scoped_ptr<const_buffer_group_t> buffers;

    friend class value_data_provider_t;
};

class large_value_data_provider_t : public value_data_provider_t {
private:
    large_value_data_provider_t(const btree_value *value, const boost::shared_ptr<transactor_t>& transactor);

public:
    virtual ~large_value_data_provider_t();

    size_t get_size() const;
    const const_buffer_group_t *get_data_as_buffers() throw (data_provider_failed_exc_t);

private:
    boost::shared_ptr<transactor_t> transactor;
    boost::scoped_ptr<const_buffer_group_t> buffers;
    boost::scoped_ptr<large_buf_t> large_value;
    union {
        large_buf_ref lb_ref;
        char lb_ref_bytes[LARGE_BUF_REF_SIZE];
    };

    friend class value_data_provider_t;
};

#endif // __BTREE_BTREE_DATA_PROVIDER_HPP__
