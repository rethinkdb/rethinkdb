#ifndef __BTREE_APPEND_PREPEND_HPP__
#define __BTREE_APPEND_PREPEND_HPP__

#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "store.hpp"

append_prepend_result_t btree_append_prepend(const store_key_t &key, btree_slice_t *slice, unique_ptr_t<data_provider_t> data, bool append, castime_t castime);

#endif // __BTREE_APPEND_PREPEND_HPP__
