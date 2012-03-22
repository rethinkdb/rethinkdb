#ifndef BTREE_APPEND_PREPEND_HPP_
#define BTREE_APPEND_PREPEND_HPP_

#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "memcached/store.hpp"

append_prepend_result_t btree_append_prepend(const store_key_t &key, btree_slice_t *slice, const boost::intrusive_ptr<data_buffer_t>& data, bool append, cas_t proposed_cas, exptime_t effective_time, repli_timestamp_t timestamp, transaction_t *txn, got_superblock_t& superblock);

#endif // BTREE_APPEND_PREPEND_HPP_
