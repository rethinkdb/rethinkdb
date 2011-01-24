#ifndef __BTREE_DELETE_HPP__
#define __BTREE_DELETE_HPP__

#include "store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

store_t::delete_result_t btree_delete(const btree_key *key, btree_slice_t *slice);

#endif // __BTREE_DELETE_HPP__
