#ifndef __BTREE_CORO_WRAPPERS_HPP__
#define __BTREE_CORO_WRAPPERS_HPP__
// XXX This file is temporary.

#include "store.hpp"
#include "buffer_cache/buffer_cache.hpp"

void co_deliver_get_result(const_buffer_group_t *bg, mcflags_t flags, cas_t cas,
    value_cond_t<store_t::get_result_t> *dest);

// XXX
// co_replicant_value()
//struct co_replicant_done_callback_t : public store_t::replicant_t::done_callback_t {
//    coro_t *self;
//    co_replicant_done_callback_t() : self(coro_t::self()) {}
//    void have_copied_value() { self->notify(); }
//    void join() { coro_t::wait(); }
//};
//
//void co_replicant_value(store_t::replicant_t *replicant, const store_key_t *key,
//                        const_buffer_group_t *value, mcflags_t flags,
//                        exptime_t exptime, cas_t cas, repli_timestamp timestamp);

#endif /* __BTREE_CORO_WRAPPERS_HPP__ */
