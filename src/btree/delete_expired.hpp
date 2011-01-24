#ifndef __BTREE_DELETE_EXPIRED_HPP__
#define __BTREE_DELETE_EXPIRED_HPP__

#include "btree/node.hpp"
#include "btree/slice.hpp"

void btree_delete_expired(const btree_key *key, btree_slice_t *slice);

#endif // __BTREE_DELETE_EXPIRED_HPP__
