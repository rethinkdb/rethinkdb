#ifndef __BTREE_CORO_WRAPPERS_HPP__
#define __BTREE_CORO_WRAPPERS_HPP__
// XXX This file is temporary.

#include "store.hpp"
#include "buffer_cache/buffer_cache.hpp"

void co_deliver_get_result(const_buffer_group_t *bg, mcflags_t flags, cas_t cas,
    value_cond_t<store_t::get_result_t> *dest);

#endif /* __BTREE_CORO_WRAPPERS_HPP__ */
