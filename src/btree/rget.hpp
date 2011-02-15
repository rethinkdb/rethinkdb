#ifndef __BTREE_RGET_HPP__
#define __BTREE_RGET_HPP__

#include "utils.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/transactor.hpp"
#include "buffer_cache/co_functions.hpp"
#include <boost/shared_ptr.hpp>

rget_result_t btree_rget_slice(btree_slice_t *slice, rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key);

#endif // __BTREE_RGET_HPP__

