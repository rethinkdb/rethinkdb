#ifndef __BTREE_WALK_HPP__
#define __BTREE_WALK_HPP__

#include "store.hpp"
#include "btree/key_value_store.hpp"

void walk_btrees(btree_key_value_store_t *store, store_t::walk_callback_t *cb);

#endif /* __BTREE_WALK_HPP__ */
