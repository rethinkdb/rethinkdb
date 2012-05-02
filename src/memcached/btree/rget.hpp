#ifndef MEMCACHED_BTREE_RGET_HPP_
#define MEMCACHED_BTREE_RGET_HPP_

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

#include "buffer_cache/types.hpp"
#include "memcached/queries.hpp"
#include "utils.hpp"

class btree_slice_t;
class superblock_t;

rget_result_t memcached_rget_slice(btree_slice_t *slice, rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key,
    exptime_t effective_time, boost::scoped_ptr<transaction_t>& txn, boost::scoped_ptr<superblock_t> &superblock);

#endif // MEMCACHED_BTREE_RGET_HPP_

