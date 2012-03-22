#ifndef BTREE_GET_HPP_
#define BTREE_GET_HPP_

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>
#include "memcached/store.hpp"
#include "btree/operations.hpp"

class btree_slice_t;

get_result_t btree_get(const store_key_t &key, btree_slice_t *slice, exptime_t effective_time, transaction_t *txn, got_superblock_t& superblock);

#endif // BTREE_GET_HPP_
