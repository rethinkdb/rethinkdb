// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_GET_HPP_
#define MEMCACHED_MEMCACHED_BTREE_GET_HPP_

#include "buffer_cache/types.hpp"
#include "memcached/queries.hpp"

class btree_slice_t;
class superblock_t;

get_result_t memcached_get(const store_key_t &key, btree_slice_t *slice,
                           exptime_t effective_time, superblock_t *superblock);

#endif // MEMCACHED_MEMCACHED_BTREE_GET_HPP_
