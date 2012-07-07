#ifndef BTREE_GET_DISTRIBUTION_HPP_
#define BTREE_GET_DISTRIBUTION_HPP_

#include <vector>

#include "btree/keys.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/types.hpp"

class superblock_t;

void get_btree_key_distribution(btree_slice_t *slice, transaction_t *txn, superblock_t *superblock, int depth_limit, int *key_count_out, std::vector<store_key_t> *keys_out);

#endif /* BTREE_GET_DISTRIBUTION_HPP_ */
