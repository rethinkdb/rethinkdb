#ifndef __BTREE_INCR_DECR_HPP__
#define __BTREE_INCR_DECR_HPP__

#include "store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

store_t::incr_decr_result_t btree_incr_decr(const btree_key *key, btree_slice_t *slice, bool increment, unsigned long long delta, cas_t proposed_cas);

#endif // __BTREE_SET_HPP__
