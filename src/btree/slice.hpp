#ifndef __BTREE_SLICE_HPP__
#define __BTREE_SLICE_HPP__

#include "store.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "serializer/translator.hpp"

class initialize_superblock_fsm_t;
struct btree_replicant_t;

namespace replication {
class masterstore_t;
}  // namespace replication

/* btree_slice_t is a thin wrapper around cache_t that handles initializing the buffer
cache for the purpose of storing a btree. There are many btree_slice_ts per
btree_key_value_store_t. */

class btree_slice_t :
    public store_t,
    public home_thread_mixin_t
{
public:
    // Blocks
    static void create(translator_serializer_t *serializer,
                       mirrored_cache_config_t *dynamic_config,
                       mirrored_cache_static_config_t *static_config,
                       replication::masterstore_t *masterstore);

    // Blocks
    btree_slice_t(translator_serializer_t *serializer,
                  mirrored_cache_config_t *dynamic_config,
                  mirrored_cache_static_config_t *static_config,
                  replication::masterstore_t *masterstore);

    // Blocks
    ~btree_slice_t();

    /* store_t interface. */
    get_result_t get(store_key_t *key);
    get_result_t get_cas(store_key_t *key, castime_t castime);
    rget_result_t rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open, uint64_t max_results);
    set_result_t set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, castime_t castime);
    incr_decr_result_t incr(store_key_t *key, unsigned long long amount, castime_t castime);
    incr_decr_result_t decr(store_key_t *key, unsigned long long amount, castime_t castime);
    append_prepend_result_t append(store_key_t *key, data_provider_t *data, castime_t castime);
    append_prepend_result_t prepend(store_key_t *key, data_provider_t *data, castime_t castime);
    delete_result_t delete_key(store_key_t *key, repli_timestamp timestamp);

    /* Used by internal btree logic */
    cache_t cache;

private:
    uint32_t cas_counter_;
    castime_t gen_castime();
    castime_t generate_if_necessary(castime_t castime);
    repli_timestamp generate_if_necessary(repli_timestamp timestamp);
    replication::masterstore_t *masterstore_;

    DISABLE_COPYING(btree_slice_t);
};

#endif /* __BTREE_SLICE_HPP__ */
