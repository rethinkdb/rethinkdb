#ifndef __BTREE_SET_HPP__
#define __BTREE_SET_HPP__

#include "store.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"

enum set_type_t {
    set_type_set,
    set_type_add,
    set_type_replace,
    set_type_cas
};

store_t::set_result_t btree_set(const btree_key *key, btree_slice_t *slice, data_provider_t *data, set_type_t type, mcflags_t mcflags, exptime_t exptime, cas_t req_cas, castime_t castime);

#endif // __BTREE_SET_HPP__
