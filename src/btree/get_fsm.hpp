#ifndef __BTREE_GET_FSM_HPP__
#define __BTREE_GET_FSM_HPP__

#include "event.hpp"
#include "btree/fsm.hpp"

void btree_get(btree_key *key, btree_key_value_store_t *store, store_t::get_callback_t *cb);
void co_btree_get(btree_key *key, btree_key_value_store_t *store, store_t::get_callback_t *cb);

#endif // __BTREE_GET_FSM_HPP__
