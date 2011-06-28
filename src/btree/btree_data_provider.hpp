#ifndef __BTREE_BTREE_DATA_PROVIDER_HPP__
#define __BTREE_BTREE_DATA_PROVIDER_HPP__

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "btree/value.hpp"
#include "data_provider.hpp"
#include "buffer_cache/blob.hpp"

class value_data_provider_t : public auto_copying_data_provider_t {
    value_data_provider_t(transaction_t *txn, btree_value_t *value);
    blob_t blob;
    buffer_group_t buffers;
    blob_acq_t acqs;
public:
    size_t get_size() const;
    const const_buffer_group_t *get_data_as_buffers();

    /* When create() returns, it is safe for the caller to invalidate "value" and it is
    safe for the caller to release the leaf node that "value" came from. */
    // TODO: Make this return a `boost::shared_ptr`.
    static value_data_provider_t *create(const btree_value_t *value, const boost::shared_ptr<transaction_t>& transaction);
};


#endif // __BTREE_BTREE_DATA_PROVIDER_HPP__
