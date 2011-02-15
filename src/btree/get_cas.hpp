#ifndef __BTREE_GET_CAS_HPP__
#define __BTREE_GET_CAS_HPP__

#include "store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

get_result_t btree_get_cas(const store_key_t &key, btree_slice_t *slice, castime_t castime);

#endif // __BTREE_GET_CAS_HPP__
