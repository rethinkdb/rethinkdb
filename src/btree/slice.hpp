#ifndef __BTREE_SLICE_HPP__
#define __BTREE_SLICE_HPP__

#include "store.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "serializer/serializer.hpp"

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
    static void create(cache_t *cache);

    // Blocks
    btree_slice_t(cache_t *cache, int64_t delete_queue_limit);

    // Blocks
    ~btree_slice_t();

    /* get_store_t interface */

    get_result_t get(const store_key_t &key, order_token_t token);
    rget_result_t rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token);

    /* set_store_t interface */

    mutation_result_t change(const mutation_t &m, castime_t castime, order_token_t token);

    /* btree_slice_t interface */

    void delete_all_keys_for_backfill(order_token_t token);

    void backfill(repli_timestamp_t since_when, backfill_callback_t *callback, order_token_t token);

    /* These store metadata for replication. There must be a better way to store this information,
    since it really doesn't belong on the btree_slice_t! TODO: Move them elsewhere. */
    void set_replication_clock(repli_timestamp_t t, order_token_t token);
    repli_timestamp_t get_replication_clock();
    void set_last_sync(repli_timestamp_t t, order_token_t token);
    repli_timestamp_t get_last_sync();
    void set_replication_master_id(uint32_t t);
    uint32_t get_replication_master_id();
    void set_replication_slave_id(uint32_t t);
    uint32_t get_replication_slave_id();

    cache_t *cache() { return cache_; }
    int64_t delete_queue_limit() { return delete_queue_limit_; }

    plain_sink_t pre_begin_transaction_sink_;

    // read and write operations are in different buckets for when
    // they go through throttling.
    order_source_t pre_begin_transaction_read_mode_source_; // bucket 0
    order_source_t pre_begin_transaction_write_mode_source_; // bucket 1

    enum { PRE_BEGIN_TRANSACTION_READ_MODE_BUCKET = 0, PRE_BEGIN_TRANSACTION_WRITE_MODE_BUCKET = 1 };

    order_checkpoint_t post_begin_transaction_checkpoint_;
private:
    cache_t *cache_;
    int64_t delete_queue_limit_;

    // We put all `order_token_t`s through this.
    order_checkpoint_t order_checkpoint_;

    // Cache account to be used when backfilling.
    boost::shared_ptr<cache_account_t> backfill_account;

    DISABLE_COPYING(btree_slice_t);
};

#endif /* __BTREE_SLICE_HPP__ */
