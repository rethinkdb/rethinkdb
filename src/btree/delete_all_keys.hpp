#ifndef __BTREE_DELETE_ALL_KEYS_HPP__
#define __BTREE_DELETE_ALL_KEYS_HPP__

class btree_slice_t;
class order_token_t;

void btree_delete_all_keys_for_backfill(btree_slice_t *slice, order_token_t token);




#endif  // __BTREE_DELETE_ALL_KEYS_HPP__
