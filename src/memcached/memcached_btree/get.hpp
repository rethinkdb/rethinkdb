// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_GET_HPP_
#define MEMCACHED_MEMCACHED_BTREE_GET_HPP_

#include "btree/slice.hpp"  // RSI: For SLICE_ALT
#include "buffer_cache/types.hpp"
#include "memcached/queries.hpp"

class btree_slice_t;
class superblock_t;

#if SLICE_ALT
get_result_t memcached_get(const store_key_t &key, btree_slice_t *slice,
                           exptime_t effective_time, superblock_t *superblock);
#else
get_result_t memcached_get(const store_key_t &key, btree_slice_t *slice, exptime_t effective_time, transaction_t *txn, superblock_t *superblock);
#endif

#endif // MEMCACHED_MEMCACHED_BTREE_GET_HPP_
