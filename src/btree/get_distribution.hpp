// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BTREE_GET_DISTRIBUTION_HPP_
#define BTREE_GET_DISTRIBUTION_HPP_

#include <vector>

#include "btree/keys.hpp"
#include "btree/slice.hpp"  // RSI: for SLICE_ALT
#include "buffer_cache/types.hpp"

class superblock_t;

void get_btree_key_distribution(btree_slice_t *slice,
#if !SLICE_ALT
                                transaction_t *txn,
#endif
                                superblock_t *superblock, int depth_limit,
                                int64_t *key_count_out,
                                std::vector<store_key_t> *keys_out);

#endif /* BTREE_GET_DISTRIBUTION_HPP_ */
