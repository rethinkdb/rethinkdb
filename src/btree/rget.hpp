#ifndef BTREE_RGET_HPP_
#define BTREE_RGET_HPP_

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

#include "utils.hpp"
#include "btree/slice.hpp"
#include "btree/operations.hpp"
#include "memcached/queries.hpp"

rget_result_t btree_rget_slice(btree_slice_t *slice, rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key,
    exptime_t effective_time, boost::scoped_ptr<transaction_t>& txn, got_superblock_t& superblock);

#endif // BTREE_RGET_HPP_

