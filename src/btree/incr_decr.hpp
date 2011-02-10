#ifndef __BTREE_INCR_DECR_HPP__
#define __BTREE_INCR_DECR_HPP__

#include "store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

incr_decr_result_t btree_incr_decr(const btree_key *key, btree_slice_t *slice, bool increment, uint64_t delta, castime_t castime);

#endif // __BTREE_SET_HPP__
