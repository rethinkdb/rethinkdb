#ifndef BTREE_SLICE_HPP_
#define BTREE_SLICE_HPP_

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "buffer_cache/types.hpp"
#include "concurrency/fifo_checker.hpp"
#include "perfmon/perfmon.hpp"

const unsigned int STARTING_ROOT_EVICTION_PRIORITY = 2 << 16;

class backfill_callback_t;
class key_tester_t;

class btree_stats_t {
public:
    explicit btree_stats_t(perfmon_collection_t *parent)
        : btree_collection(),
          btree_collection_membership(parent, &btree_collection, "btree"),
          pm_keys_read(secs_to_ticks(1)),
          pm_keys_set(secs_to_ticks(1)),
          pm_keys_expired(secs_to_ticks(1)),
          pm_keys_membership(&btree_collection,
              &pm_keys_read, "keys_read",
              &pm_keys_set, "keys_set",
              &pm_keys_expired, "keys_expired",
              NULL)
    { }

    perfmon_collection_t btree_collection;
    perfmon_collection_t::membership_t btree_collection_membership;
    perfmon_rate_monitor_t
        pm_keys_read,
        pm_keys_set,
        pm_keys_expired;
    perfmon_multi_membership_t pm_keys_membership;
};

/* btree_slice_t is a thin wrapper around cache_t that handles initializing the buffer
cache for the purpose of storing a btree. There are many btree_slice_ts per
btree_key_value_store_t. */

class btree_slice_t :
    public home_thread_mixin_t
{
public:
    // Blocks
    static void create(cache_t *cache);

    // Blocks
    btree_slice_t(cache_t *cache, perfmon_collection_t *parent);

    // Blocks
    ~btree_slice_t();

    cache_t *cache() { return cache_; }
    boost::shared_ptr<cache_account_t> get_backfill_account() { return backfill_account; }

    plain_sink_t pre_begin_transaction_sink_;

    // read and write operations are in different buckets for when
    // they go through throttling.
    order_source_t pre_begin_transaction_read_mode_source_; // bucket 0
    order_source_t pre_begin_transaction_write_mode_source_; // bucket 1

    enum { PRE_BEGIN_TRANSACTION_READ_MODE_BUCKET = 0, PRE_BEGIN_TRANSACTION_WRITE_MODE_BUCKET = 1 };

    order_checkpoint_t post_begin_transaction_checkpoint_;
    // We put all `order_token_t`s through this.
    order_checkpoint_t order_checkpoint_;

    btree_stats_t stats;

private:
    cache_t *cache_;

    // Cache account to be used when backfilling.
    boost::shared_ptr<cache_account_t> backfill_account;

    DISABLE_COPYING(btree_slice_t);

    //Information for cache eviction
public:
    eviction_priority_t root_eviction_priority;
};

#endif /* BTREE_SLICE_HPP_ */
