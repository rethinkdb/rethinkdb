#ifndef __BTREE_APPEND_PREPEND_HPP__
#define __BTREE_APPEND_PREPEND_HPP__

#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "memcached/store.hpp"

append_prepend_result_t btree_append_prepend(const store_key_t &key, btree_slice_t *slice, const boost::intrusive_ptr<data_buffer_t>& data, bool append, castime_t castime, order_token_t token);
append_prepend_result_t btree_append_prepend(const store_key_t &key, btree_slice_t *slice, const boost::intrusive_ptr<data_buffer_t>& data, bool append, castime_t castime, order_token_t token,
    const boost::scoped_ptr<transaction_t>& txn, got_superblock_t& superblock);

#endif // __BTREE_APPEND_PREPEND_HPP__
