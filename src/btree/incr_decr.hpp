#ifndef __BTREE_INCR_DECR_HPP__
#define __BTREE_INCR_DECR_HPP__

#include "store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

class sequence_group_t;

incr_decr_result_t btree_incr_decr(const store_key_t &key, btree_slice_t *slice, sequence_group_t *seq_group, bool increment, uint64_t delta, castime_t castime, order_token_t token);

#endif // __BTREE_SET_HPP__
