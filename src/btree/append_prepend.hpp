#ifndef __BTREE_APPEND_PREPEND_HPP__
#define __BTREE_APPEND_PREPEND_HPP__

#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "store.hpp"

store_t::append_prepend_result_t btree_append_prepend(const btree_key *key, btree_slice_t *slice, data_provider_t *data, bool append, castime_t castime);

#endif // __BTREE_APPEND_PREPEND_HPP__
