#ifndef __BTREE_SLICE_HPP__
#define __BTREE_SLICE_HPP__

#include "store.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "serializer/translator.hpp"

class backfill_callback_t;
class sequence_group_t;

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
                       mirrored_cache_static_config_t *static_config, int slice_num);

    // Blocks
    btree_slice_t(translator_serializer_t *serializer,
                  mirrored_cache_config_t *dynamic_config,
                  int64_t delete_queue_limit,
                  int slice_num);

    // Blocks
    ~btree_slice_t();

    /* get_store_t interface */

    get_result_t get(const store_key_t &key, sequence_group_t *seq_group, order_token_t token);
    rget_result_t rget(sequence_group_t *seq_gorup, rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token);

    /* set_store_t interface */

    mutation_result_t change(sequence_group_t *seq_group, const mutation_t &m, castime_t castime, order_token_t token);

    /* btree_slice_t interface */

    void delete_all_keys_for_backfill(sequence_group_t *seq_group, order_token_t token);

    void backfill(sequence_group_t *seq_group, repli_timestamp since_when, backfill_callback_t *callback, order_token_t token);

    /* These store metadata for replication. There must be a better way to store this information,
    since it really doesn't belong on the btree_slice_t! TODO: Move them elsewhere. */
    void set_replication_clock(sequence_group_t *seq_group, repli_timestamp_t t, order_token_t token);
    repli_timestamp get_replication_clock(sequence_group_t *seq_group);
    void set_last_sync(repli_timestamp_t t, order_token_t token);
    repli_timestamp get_last_sync();
    void set_replication_master_id(uint32_t t);
    uint32_t get_replication_master_id();
    void set_replication_slave_id(uint32_t t);
    uint32_t get_replication_slave_id();

    cache_t *cache() { return &cache_; }
    int64_t delete_queue_limit() { return delete_queue_limit_; }

    // Please use this only for debugging.
    const char *name() const { return informal_name_.c_str(); }


    plain_sink_t pre_begin_transaction_sink_;

    // read and write operations are in different buckets for when
    // they go through throttling.
    order_source_t pre_begin_transaction_read_mode_source_; // bucket 0
    order_source_t pre_begin_transaction_write_mode_source_; // bucket 1

    enum { PRE_BEGIN_TRANSACTION_READ_MODE_BUCKET = 0, PRE_BEGIN_TRANSACTION_WRITE_MODE_BUCKET = 1 };

    order_sink_t post_begin_transaction_sink_;
    order_source_t post_begin_transaction_source_;

private:
    cache_t cache_;
    int64_t delete_queue_limit_;

    const std::string informal_name_;

    // Cache account to be used when backfilling.
    boost::shared_ptr<cache_account_t> backfill_account;

    plain_sink_t order_sink_;

    DISABLE_COPYING(btree_slice_t);
};

#endif /* __BTREE_SLICE_HPP__ */
