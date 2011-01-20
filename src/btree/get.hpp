#ifndef __BTREE_GET_HPP__
#define __BTREE_GET_HPP__

#include "utils.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/key_value_store.hpp"

store_t::get_result_t btree_get(const btree_key *key, btree_key_value_store_t *store);

#endif // __BTREE_GET_HPP__
