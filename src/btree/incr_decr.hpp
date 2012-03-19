#ifndef BTREE_INCR_DECR_HPP_
#define BTREE_INCR_DECR_HPP_

#include "memcached/store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

incr_decr_result_t btree_incr_decr(const store_key_t &key, btree_slice_t *slice, bool increment, uint64_t delta, castime_t castime, order_token_t token);
incr_decr_result_t btree_incr_decr(const store_key_t &key, btree_slice_t *slice, bool increment, uint64_t delta, castime_t castime, transaction_t *txn, got_superblock_t& superblock);

#endif // BTREE_SET_HPP_
