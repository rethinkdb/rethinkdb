#ifndef __BTREE_GET_HPP__
#define __BTREE_GET_HPP__

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>
#include "memcached/store.hpp"
#include "btree/operations.hpp"

class btree_slice_t;

class sequence_group_t;
get_result_t btree_get(const store_key_t &key, btree_slice_t *slice, sequence_group_t *seq_group, order_token_t token);
get_result_t btree_get(const store_key_t &key, btree_slice_t *slice, order_token_t token, transaction_t *txn, got_superblock_t& superblock);

#endif // __BTREE_GET_HPP__
