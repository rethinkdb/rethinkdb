#ifndef __BTREE_SLICE_HPP__
#define __BTREE_SLICE_HPP__

#include "store.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "serializer/translator.hpp"

class initialize_superblock_fsm_t;
struct btree_replicant_t;

/* btree_slice_t is a thin wrapper around cache_t that handles initializing the buffer
cache for the purpose of storing a btree. There are many btree_slice_ts per
btree_key_value_store_t. */

class btree_slice_t :
    public store_t,
    public home_thread_mixin_t
{
public:
    // Blocks
    static void create(
        translator_serializer_t *serializer,
        mirrored_cache_config_t *dynamic_config,
        mirrored_cache_static_config_t *static_config);

    // Blocks
    btree_slice_t(
        translator_serializer_t *serializer,
        mirrored_cache_config_t *dynamic_config,
        mirrored_cache_static_config_t *static_config);

    // Blocks
    ~btree_slice_t();

public:
    /* store_t interface. */
    get_result_t get(store_key_t *key);
    get_result_t get_cas(store_key_t *key, cas_t proposed_cas);
    set_result_t set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas);
    set_result_t add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas);
    set_result_t replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas);
    set_result_t cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, cas_t proposed_cas);
    incr_decr_result_t incr(store_key_t *key, unsigned long long amount, cas_t proposed_cas);
    incr_decr_result_t decr(store_key_t *key, unsigned long long amount, cas_t proposed_cas);
    append_prepend_result_t append(store_key_t *key, data_provider_t *data, cas_t proposed_cas);
    append_prepend_result_t prepend(store_key_t *key, data_provider_t *data, cas_t proposed_cas);
    delete_result_t delete_key(store_key_t *key);

public:
    /* Used by internal btree logic */
    cache_t cache;

private:
    DISABLE_COPYING(btree_slice_t);
};

#endif /* __BTREE_SLICE_HPP__ */
