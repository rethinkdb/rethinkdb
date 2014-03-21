// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_SET_HPP_
#define MEMCACHED_MEMCACHED_BTREE_SET_HPP_

#include "btree/node.hpp"
#include "concurrency/promise.hpp"
#include "memcached/queries.hpp"

class btree_slice_t;
class superblock_t;

set_result_t memcached_set(const store_key_t &key,
                           btree_slice_t *slice,
                           const counted_t<data_buffer_t>& data,
                           mcflags_t mcflags,
                           exptime_t exptime,
                           add_policy_t add_policy,
                           replace_policy_t replace_policy,
                           cas_t req_cas,
                           cas_t proposed_cas,
                           exptime_t effective_time,
                           repli_timestamp_t timestamp,
                           superblock_t *superblock,
                           promise_t<superblock_t *> *pass_back_superblock = NULL);

#endif // MEMCACHED_MEMCACHED_BTREE_SET_HPP_
