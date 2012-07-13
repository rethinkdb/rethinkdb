#ifndef MEMCACHED_BTREE_DISTRIBUTION_HPP_
#define MEMCACHED_BTREE_DISTRIBUTION_HPP_

#include "memcached/queries.hpp"
#include "btree/slice.hpp"

class superblock_t;

distribution_result_t memcached_distribution_get(btree_slice_t *slice, int max_depth, const store_key_t &left_key, 
        exptime_t effective_time, boost::scoped_ptr<transaction_t> *txn, superblock_t *superblock);

#endif /* MEMCACHED_BTREE_DISTRIBUTION_HPP_ */
