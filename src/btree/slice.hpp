#ifndef __BTREE_SLICE_HPP__
#define __BTREE_SLICE_HPP__

#include "store.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "serializer/translator.hpp"

class backfill_callback_t;

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
                       mirrored_cache_static_config_t *static_config);

    // Blocks
    btree_slice_t(translator_serializer_t *serializer,
                  mirrored_cache_config_t *dynamic_config,
                  int64_t delete_queue_limit);

    // Blocks
    ~btree_slice_t();

    /* get_store_t interface. */

    get_result_t get(const store_key_t &key);
    rget_result_t rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key);

    /* set_store_t interface */

    mutation_result_t change(const mutation_t &m, castime_t castime);

    /* btree_slice_t interface */

    void delete_all_keys_for_backfill();

    void backfill(repli_timestamp since_when, backfill_callback_t *callback);

    /* These store metadata for replication. There must be a better way to store this information,
    since it really doesn't belong on the btree_slice_t! TODO: Move them elsewhere. */
    void set_replication_clock(repli_timestamp_t t);
    repli_timestamp get_replication_clock();
    void set_last_sync(repli_timestamp_t t);
    repli_timestamp get_last_sync();
    void set_replication_master_id(uint32_t t);
    uint32_t get_replication_master_id();
    void set_replication_slave_id(uint32_t t);
    uint32_t get_replication_slave_id();

    cache_t *cache() { return &cache_; }
    int64_t delete_queue_limit() { return delete_queue_limit_; }

private:
    cache_t cache_;
    int64_t delete_queue_limit_;

    DISABLE_COPYING(btree_slice_t);
};

#endif /* __BTREE_SLICE_HPP__ */
