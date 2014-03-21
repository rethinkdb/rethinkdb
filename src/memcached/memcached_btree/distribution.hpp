// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_DISTRIBUTION_HPP_
#define MEMCACHED_MEMCACHED_BTREE_DISTRIBUTION_HPP_

#include "memcached/queries.hpp"

class superblock_t;

distribution_result_t memcached_distribution_get(int max_depth,
                                                 const store_key_t &left_key,
                                                 exptime_t effective_time,
                                                 superblock_t *superblock);

#endif /* MEMCACHED_MEMCACHED_BTREE_DISTRIBUTION_HPP_ */
