#ifndef MEMCACHED_BTREE_DELETE_HPP_
#define MEMCACHED_BTREE_DELETE_HPP_

#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "memcached/queries.hpp"

class superblock_t;

delete_result_t memcached_delete(const store_key_t &key, bool dont_put_in_delete_queue, btree_slice_t *slice, exptime_t effective_time, repli_timestamp_t timestamp, transaction_t *txn, superblock_t *superblock);

#endif // MEMCACHED_BTREE_DELETE_HPP_
