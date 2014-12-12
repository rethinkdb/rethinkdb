// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_SLICE_HPP_
#define BTREE_SLICE_HPP_

#include <string>
#include <vector>

#include "buffer_cache/types.hpp"
#include "buffer_cache/cache_account.hpp"
#include "containers/scoped.hpp"
#include "perfmon/perfmon.hpp"

class cache_account_t;
class backfill_callback_t;
class binary_blob_t;
class buf_lock_t;
class buf_parent_t;
class cache_t;
class key_tester_t;

enum class index_type_t {
    PRIMARY,
    SECONDARY
};

class btree_stats_t {
public:
    explicit btree_stats_t(perfmon_collection_t *parent,
                           const std::string &identifier,
                           index_type_t index_type)
        : btree_collection(),
          pm_keys_read(secs_to_ticks(1)),
          pm_keys_set(secs_to_ticks(1)),
          pm_keys_membership(&btree_collection,
              &pm_keys_read, "keys_read",
              &pm_total_keys_read, "total_keys_read",
              &pm_keys_set, "keys_set",
              &pm_total_keys_set, "total_keys_set") {
        if (parent != NULL) {
            rename(parent, identifier, index_type);
        }
    }

    void hide() {
        btree_collection_membership.reset();
    }

    void rename(perfmon_collection_t *parent,
                const std::string &identifier,
                index_type_t index_type) {
        btree_collection_membership.reset();
        btree_collection_membership.init(new perfmon_membership_t(
            parent,
            &btree_collection,
            (index_type == index_type_t::PRIMARY ?
            "btree-" : "btree-index-") + identifier));
    }

    perfmon_collection_t btree_collection;
    scoped_ptr_t<perfmon_membership_t> btree_collection_membership;
    perfmon_rate_monitor_t
        pm_keys_read,
        pm_keys_set;
    perfmon_counter_t
        pm_total_keys_read,
        pm_total_keys_set;
    perfmon_multi_membership_t pm_keys_membership;
};

/* btree_slice_t is a thin wrapper around cache_t that handles initializing the buffer
cache for the purpose of storing a btree. There are many btree_slice_ts per
btree_key_value_store_t. */

class btree_slice_t : public home_thread_mixin_debug_only_t {
public:
    // Initializes a superblock (presumably, a buf_lock_t constructed with
    // alt_create_t::create) for use with btrees, setting the initial value of the
    // metainfo (with a single key/value pair).
    static void init_superblock(buf_lock_t *superblock,
                                const std::vector<char> &metainfo_key,
                                const binary_blob_t &metainfo_value);

    btree_slice_t(cache_t *cache,
                  perfmon_collection_t *parent,
                  const std::string &identifier,
                  index_type_t index_type);

    ~btree_slice_t();

    cache_t *cache() { return cache_; }
    cache_account_t *get_backfill_account() { return &backfill_account_; }

    btree_stats_t stats;

private:
    cache_t *cache_;

    // Cache account to be used when backfilling.
    cache_account_t backfill_account_;

    DISABLE_COPYING(btree_slice_t);
};

#endif /* BTREE_SLICE_HPP_ */
