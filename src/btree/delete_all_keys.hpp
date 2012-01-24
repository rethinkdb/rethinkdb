#ifndef __BTREE_DELETE_ALL_KEYS_HPP__
#define __BTREE_DELETE_ALL_KEYS_HPP__

class btree_slice_t;
class order_token_t;
class sequence_group_t;

void btree_delete_all_keys_for_backfill(btree_slice_t *slice, sequence_group_t *seq_group, order_token_t token);




#endif  // __BTREE_DELETE_ALL_KEYS_HPP__
