#ifndef __BTREE_GET_CAS_HPP__
#define __BTREE_GET_CAS_HPP__

#include "memcached/store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

class sequence_group_t;

get_result_t btree_get_cas(const store_key_t &key, btree_slice_t *slice, sequence_group_t *seq_group, castime_t castime, order_token_t token);

#endif // __BTREE_GET_CAS_HPP__
