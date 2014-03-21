// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_INCR_DECR_HPP_
#define MEMCACHED_MEMCACHED_BTREE_INCR_DECR_HPP_

#include "btree/node.hpp"
#include "memcached/queries.hpp"

class btree_slice_t;
class superblock_t;

incr_decr_result_t
memcached_incr_decr(const store_key_t &key, btree_slice_t *slice, bool increment,
                    uint64_t delta, cas_t proposed_cas, exptime_t effective_time,
                    repli_timestamp_t timestamp, superblock_t *superblock);

#endif // MEMCACHED_MEMCACHED_BTREE_INCR_DECR_HPP_
