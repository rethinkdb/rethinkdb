#ifndef MEMCACHED_BTREE_INCR_DECR_HPP_
#define MEMCACHED_BTREE_INCR_DECR_HPP_

#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "memcached/queries.hpp"

incr_decr_result_t memcached_incr_decr(const store_key_t &key, btree_slice_t *slice, bool increment, uint64_t delta, cas_t proposed_cas, exptime_t effective_time, repli_timestamp_t timestamp, transaction_t *txn, got_superblock_t& superblock);

#endif // MEMCACHED_BTREE_SET_HPP_
