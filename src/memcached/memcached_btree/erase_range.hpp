#ifndef MEMCACHED_MEMCACHED_BTREE_ERASE_RANGE_HPP_
#define MEMCACHED_MEMCACHED_BTREE_ERASE_RANGE_HPP_

#include "btree/erase_range.hpp"

void memcached_erase_range(btree_slice_t *slice, key_tester_t *tester,
                                 bool left_key_supplied, const store_key_t& left_key_exclusive,
                                 bool right_key_supplied, const store_key_t& right_key_inclusive,
                                 transaction_t *txn, superblock_t *superblock);

void memcached_erase_range(btree_slice_t *slice, key_tester_t *tester,
                                 const key_range_t &keys,
                                 transaction_t *txn, superblock_t *superblock);

#endif /* MEMCACHED_MEMCACHED_BTREE_ERASE_RANGE_HPP_ */
