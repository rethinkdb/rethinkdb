#ifndef __BTREE_GET_HPP__
#define __BTREE_GET_HPP__

#include "memcached/store.hpp"

class btree_slice_t;

get_result_t btree_get(const store_key_t &key, btree_slice_t *slice, order_token_t token);

#endif // __BTREE_GET_HPP__
