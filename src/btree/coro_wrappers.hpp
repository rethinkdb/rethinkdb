#ifndef __BTREE_CORO_WRAPPERS_HPP__
#define __BTREE_CORO_WRAPPERS_HPP__
// XXX This file is temporary.

#include "store.hpp"
#include "buffer_cache/buffer_cache.hpp"

// co_get_data_provider_value()
struct co_data_provider_done_callback_t : public data_provider_t::done_callback_t {
    coro_t *self;
    bool success;

    // Calling notify() on an active coroutine should probably be an error, but
    // it works for this, for now.

    co_data_provider_done_callback_t() : self(coro_t::self()) {}

    void have_provided_value() {
        success = true;
        self->notify();
    }

    void have_failed() {
        success = false;
        self->notify();
    }

    bool join() {
        coro_t::wait();
        return success;
    }
};

bool co_get_data_provider_value(data_provider_t *data, buffer_group_t *dest);

// co_value()
struct value_done_t : public store_t::get_callback_t::done_callback_t {
    coro_t *self;
    value_done_t() : self(coro_t::self()) { }
    void have_copied_value() { self->notify(); }
};

void co_value(store_t::get_callback_t *cb, const_buffer_group_t *value_buffers, mcflags_t flags, cas_t cas);

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
