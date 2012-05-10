#ifndef BUFFER_CACHE_MIRRORED_STATS_HPP_
#define BUFFER_CACHE_MIRRORED_STATS_HPP_

#include "perfmon.hpp"
#include "stats/persist.hpp"

/* A class to hold all the stats we care about for this cache. */
struct mc_cache_stats_t {
    perfmon_collection_t cache_collection;

    mc_cache_stats_t(perfmon_collection_t *parent);

    perfmon_counter_t 
        pm_registered_snapshots,
        pm_registered_snapshot_blocks;

    perfmon_sampler_t pm_snapshots_per_transaction;

    perfmon_persistent_counter_t 
        pm_cache_hits,
        pm_cache_misses;

    perfmon_duration_sampler_t
        pm_bufs_acquiring,
        pm_bufs_held;

    perfmon_sampler_t pm_patches_size_per_write;

    perfmon_duration_sampler_t
        pm_transactions_starting,
        pm_transactions_active,
        pm_transactions_committing;


    /* Used in writeback.hpp */
    perfmon_duration_sampler_t
        pm_flushes_diff_flush,
        pm_flushes_diff_store,
        pm_flushes_locking,
        pm_flushes_writing;

    perfmon_sampler_t
        pm_flushes_blocks,
        pm_flushes_blocks_dirty,
        pm_flushes_diff_patches_stored,
        pm_flushes_diff_storage_failures;

    perfmon_counter_t
        pm_n_blocks_in_memory,
        pm_n_blocks_dirty,
        pm_n_blocks_total;

    perfmon_sampler_t pm_patches_size_ratio;

    // used in buffer_cache/mirrored/page_repl_random.cc
    perfmon_counter_t pm_n_blocks_evicted;

};

#endif // BUFFER_CACHE_MIRRORED_STATS_HPP_
