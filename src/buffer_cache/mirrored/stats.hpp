// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_MIRRORED_STATS_HPP_
#define BUFFER_CACHE_MIRRORED_STATS_HPP_

#include "perfmon/perfmon.hpp"

/* A class to hold all the stats we care about for this cache. */
struct mc_cache_stats_t {
    perfmon_collection_t cache_collection;
    perfmon_membership_t cache_membership;

    explicit mc_cache_stats_t(perfmon_collection_t *parent);

    perfmon_counter_t
        pm_registered_snapshots,
        pm_registered_snapshot_blocks;

    perfmon_sampler_t pm_snapshots_per_transaction;

    perfmon_counter_t
        pm_cache_hits,
        pm_cache_misses;

    perfmon_duration_sampler_t
        pm_bufs_acquiring,
        pm_bufs_held;

    perfmon_duration_sampler_t
        pm_transactions_starting,
        pm_transactions_active,
        pm_transactions_committing;


    /* Used in writeback.hpp */
    perfmon_duration_sampler_t
        pm_flushes_locking,
        pm_flushes_writing;

    perfmon_sampler_t
        pm_flushes_blocks,
        pm_flushes_blocks_dirty;

    perfmon_counter_t
        pm_n_blocks_in_memory,
        pm_n_blocks_dirty,
        pm_n_blocks_total;

    // used in buffer_cache/mirrored/page_repl_random.cc
    perfmon_counter_t pm_n_blocks_evicted;

    /* This is for exposing the block size */
    struct perfmon_cache_custom_t : public perfmon_t {
    public:
        perfmon_cache_custom_t();
        void *begin_stats();
        void visit_stats(void *);
        scoped_ptr_t<perfmon_result_t> end_stats(void *);
    public:
        uint32_t block_size;
    };
    perfmon_cache_custom_t pm_block_size;

    perfmon_multi_membership_t cache_collection_membership;
};

#endif // BUFFER_CACHE_MIRRORED_STATS_HPP_
