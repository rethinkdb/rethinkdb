#include "btree/slice.hpp"
#include "btree/node.hpp"
#include "buffer_cache/transactor.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "concurrency/cond_var.hpp"
#include "btree/get.hpp"
#include "btree/set.hpp"
#include "btree/incr_decr.hpp"
#include "btree/append_prepend.hpp"
#include "btree/delete.hpp"
#include "btree/get_cas.hpp"
#include <boost/scoped_ptr.hpp>

void btree_slice_t::create(
    translator_serializer_t *serializer,
    mirrored_cache_config_t *dynamic_config,
    mirrored_cache_static_config_t *static_config)
{
    /* Put slice in a scoped pointer because it's way to big to allocate on a coroutine stack */
    boost::scoped_ptr<btree_slice_t> slice(new btree_slice_t(serializer, dynamic_config, static_config));

    /* Initialize the root block */
    transactor_t transactor(&slice->cache, rwi_write);
    buf_lock_t superblock(transactor, SUPERBLOCK_ID, rwi_write);
    btree_superblock_t *sb = (btree_superblock_t*)(superblock.buf()->get_data_major_write());
    sb->magic = btree_superblock_t::expected_magic;
    sb->root_block = NULL_BLOCK_ID;

    // Destructors handle cleanup and stuff
}

btree_slice_t::btree_slice_t(
        translator_serializer_t *serializer,
        mirrored_cache_config_t *dynamic_config,
        mirrored_cache_static_config_t *static_config) :
    cache(serializer, dynamic_config, static_config)
{
    // Start up cache
    struct : public cache_t::ready_callback_t, public cond_t {
        void on_cache_ready() { pulse(); }
    } ready_cb;
    if (!cache.start(&ready_cb)) ready_cb.wait();
}

btree_slice_t::~btree_slice_t() {
    // Shut down cache
    struct : public cache_t::shutdown_callback_t, public cond_t {
        void on_cache_shutdown() { pulse(); }
    } shutdown_cb;
    if (!cache.shutdown(&shutdown_cb)) shutdown_cb.wait();
}

store_t::get_result_t btree_slice_t::get(store_key_t *key) {
    return btree_get(key, this);
}

store_t::get_result_t btree_slice_t::get_cas(store_key_t *key, cas_t proposed_cas) {
    return btree_get_cas(key, this, proposed_cas);
}

store_t::set_result_t btree_slice_t::set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas) {
    return btree_set(key, this, data, set_type_set, flags, exptime, 0, proposed_cas);
}

store_t::set_result_t btree_slice_t::add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas) {
    return btree_set(key, this, data, set_type_add, flags, exptime, 0, proposed_cas);
}

store_t::set_result_t btree_slice_t::replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas) {
    return btree_set(key, this, data, set_type_replace, flags, exptime, 0, proposed_cas);
}

store_t::set_result_t btree_slice_t::cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, cas_t proposed_cas) {
    return btree_set(key, this, data, set_type_cas, flags, exptime, unique, proposed_cas);
}

store_t::incr_decr_result_t btree_slice_t::incr(store_key_t *key, unsigned long long amount, cas_t proposed_cas) {
    return btree_incr_decr(key, this, true, amount, proposed_cas);
}

store_t::incr_decr_result_t btree_slice_t::decr(store_key_t *key, unsigned long long amount, cas_t proposed_cas) {
    return btree_incr_decr(key, this, false, amount, proposed_cas);
}

store_t::append_prepend_result_t btree_slice_t::append(store_key_t *key, data_provider_t *data, cas_t proposed_cas) {
    return btree_append_prepend(key, this, data, true, proposed_cas);
}

store_t::append_prepend_result_t btree_slice_t::prepend(store_key_t *key, data_provider_t *data, cas_t proposed_cas) {
    return btree_append_prepend(key, this, data, false, proposed_cas);
}

store_t::delete_result_t btree_slice_t::delete_key(store_key_t *key) {
    return btree_delete(key, this);
}
