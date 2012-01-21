#ifndef __BTREE_SET_HPP__
#define __BTREE_SET_HPP__

#include "memcached/store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

set_result_t btree_set(const store_key_t &key, btree_slice_t *slice,
                       const boost::intrusive_ptr<data_buffer_t>& data, mcflags_t mcflags, exptime_t exptime,
                       add_policy_t add_policy, replace_policy_t replace_policy, cas_t req_cas,
                       castime_t castime, order_token_t token);
set_result_t btree_set(const store_key_t &key, btree_slice_t *slice,
                       const boost::intrusive_ptr<data_buffer_t>& data, mcflags_t mcflags, exptime_t exptime,
                       add_policy_t add_policy, replace_policy_t replace_policy, cas_t req_cas,
                       castime_t castime, order_token_t token,
                       transaction_t *txn, got_superblock_t& superblock);

#endif // __BTREE_SET_HPP__
