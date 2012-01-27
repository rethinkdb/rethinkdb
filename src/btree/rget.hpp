#ifndef __BTREE_RGET_HPP__
#define __BTREE_RGET_HPP__

#include "utils.hpp"
#include "btree/slice.hpp"


class sequence_group_t;

rget_result_t btree_rget_slice(btree_slice_t *slice, sequence_group_t *seq_group, rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token);

#endif // __BTREE_RGET_HPP__

