#ifndef __BTREE_GET_CAS_HPP__
#define __BTREE_GET_CAS_HPP__

#include "memcached/store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

get_result_t btree_get_cas(const store_key_t &key, btree_slice_t *slice, castime_t castime, order_token_t token);
get_result_t btree_get_cas(const store_key_t &key, btree_slice_t *slice, castime_t castime, transaction_t *txn, got_superblock_t& superblock);

#endif // __BTREE_GET_CAS_HPP__
