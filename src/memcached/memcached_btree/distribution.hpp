// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_DISTRIBUTION_HPP_
#define MEMCACHED_MEMCACHED_BTREE_DISTRIBUTION_HPP_

#include "memcached/queries.hpp"
#include "btree/slice.hpp"  // RSI: for SLICE_ALT?

class superblock_t;

#if SLICE_ALT
distribution_result_t memcached_distribution_get(btree_slice_t *slice, int max_depth,
                                                 const store_key_t &left_key,
                                                 exptime_t effective_time,
                                                 superblock_t *superblock);
#else
distribution_result_t memcached_distribution_get(btree_slice_t *slice, int max_depth, const store_key_t &left_key,
        exptime_t effective_time, transaction_t *txn, superblock_t *superblock);
#endif

#endif /* MEMCACHED_MEMCACHED_BTREE_DISTRIBUTION_HPP_ */
