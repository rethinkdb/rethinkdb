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
    public get_store_t,
    public set_store_t,
    public home_thread_mixin_t
{
public:
    // Blocks
    static void create(translator_serializer_t *serializer,
                       mirrored_cache_config_t *config);

    // Blocks
    btree_slice_t(translator_serializer_t *serializer,
                  mirrored_cache_config_t *config);

    // Blocks
    ~btree_slice_t();

    /* get_store_t interface. */

    get_result_t get(store_key_t *key);
    rget_result_t rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open);

    /* set_store_t interface */

    get_result_t get_cas(store_key_t *key, castime_t castime);
    
    set_result_t sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime,
                      add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas);
    delete_result_t delete_key(store_key_t *key, repli_timestamp timestamp);

    incr_decr_result_t incr_decr(incr_decr_kind_t kind, store_key_t *key, uint64_t amount, castime_t castime);
    append_prepend_result_t append_prepend(append_prepend_kind_t kind, store_key_t *key, data_provider_t *data, castime_t castime);

    /* For internal use */
    cache_t& cache() { return cache_; }

private:
    cache_t cache_;

    DISABLE_COPYING(btree_slice_t);
};

// Stats

extern perfmon_duration_sampler_t
    pm_cmd_set,
    pm_cmd_get,
    pm_cmd_get_without_threads,
    pm_cmd_rget;

#endif /* __BTREE_SLICE_HPP__ */
