// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_RGET_HPP_
#define MEMCACHED_MEMCACHED_BTREE_RGET_HPP_

#include "buffer_cache/types.hpp"
#include "memcached/queries.hpp"
#include "utils.hpp"

class superblock_t;

static const size_t rget_max_chunk_size = MEGABYTE;

size_t estimate_rget_result_pair_size(const key_with_data_buffer_t &pair);

rget_result_t memcached_rget_slice(const key_range_t &range,
                                   int maximum, exptime_t effective_time,
                                   superblock_t *superblock);

#endif // MEMCACHED_MEMCACHED_BTREE_RGET_HPP_

