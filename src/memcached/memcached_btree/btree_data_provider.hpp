// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_BTREE_DATA_PROVIDER_HPP_
#define MEMCACHED_MEMCACHED_BTREE_BTREE_DATA_PROVIDER_HPP_

#include "buffer_cache/types.hpp"
#include "containers/counted.hpp"

class alt_buf_parent_t;
struct memcached_value_t;
struct data_buffer_t;

counted_t<data_buffer_t> value_to_data_buffer(const memcached_value_t *value,
                                              alt_buf_parent_t parent);

#endif // MEMCACHED_MEMCACHED_BTREE_BTREE_DATA_PROVIDER_HPP_
