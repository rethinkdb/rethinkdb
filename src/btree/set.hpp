#ifndef __BTREE_SET_HPP__
#define __BTREE_SET_HPP__

#include "store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

set_result_t btree_set(const btree_key *key, btree_slice_t *slice,
    data_provider_t *data, mcflags_t mcflags, exptime_t exptime,
    add_policy_t add_policy, replace_policy_t replace_policy, cas_t req_cas,
    castime_t castime);

#endif // __BTREE_SET_HPP__
