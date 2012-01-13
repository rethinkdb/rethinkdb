#ifndef __BTREE_DELETE_HPP__
#define __BTREE_DELETE_HPP__

#include "store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

class sequence_group_t;

delete_result_t btree_delete(const store_key_t &key, bool dont_put_in_delete_queue, btree_slice_t *slice, sequence_group_t *seq_group, repli_timestamp timestamp, order_token_t token);

#endif // __BTREE_DELETE_HPP__
