#ifndef BTREE_DELETE_EXPIRED_HPP_
#define BTREE_DELETE_EXPIRED_HPP_

#include "btree/node.hpp"
#include "btree/slice.hpp"

void btree_delete_expired(const store_key_t &key, btree_slice_t *slice);

#endif // BTREE_DELETE_EXPIRED_HPP_
