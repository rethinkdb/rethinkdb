#ifndef MEMCACHED_BTREE_GET_CAS_HPP_
#define MEMCACHED_BTREE_GET_CAS_HPP_

#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "memcached/queries.hpp"

get_result_t memcached_get_cas(const store_key_t &key, btree_slice_t *slice, cas_t proposed_cas, exptime_t effective_time, repli_timestamp_t timestamp, transaction_t *txn, got_superblock_t& superblock);

#endif // MEMCACHED_BTREE_GET_CAS_HPP_
