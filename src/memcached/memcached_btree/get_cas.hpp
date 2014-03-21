// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_GET_CAS_HPP_
#define MEMCACHED_MEMCACHED_BTREE_GET_CAS_HPP_

#include "btree/node.hpp"
#include "memcached/queries.hpp"

class btree_slice_t;
class superblock_t;

get_result_t memcached_get_cas(const store_key_t &key, btree_slice_t *slice,
                               cas_t proposed_cas, exptime_t effective_time,
                               repli_timestamp_t timestamp,
                               superblock_t *superblock);

#endif // MEMCACHED_MEMCACHED_BTREE_GET_CAS_HPP_
