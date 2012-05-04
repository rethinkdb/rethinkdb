#include "buffer_cache/mirrored/stats.hpp"

mc_cache_stats_t::mc_cache_stats_t(perfmon_collection_t *parent)
    : cache_collection("cache", parent),
      pm_registered_snapshots("registered_snapshots", &cache_collection),
      pm_registered_snapshot_blocks("registered_snapshot_blocks", &cache_collection),
      pm_snapshots_per_transaction("snapshots_per_transaction", secs_to_ticks(1), false, &cache_collection),
      pm_cache_hits("cache_hits", &cache_collection),
      pm_cache_misses("cache_misses", &cache_collection),
      pm_bufs_acquiring("bufs_acquiring", secs_to_ticks(1), &cache_collection),
      pm_bufs_held("bufs_held", secs_to_ticks(1), &cache_collection),
      pm_patches_size_per_write("patches_size_per_write_buf", secs_to_ticks(1), false, &cache_collection),
      pm_transactions_starting("transactions_starting", secs_to_ticks(1), &cache_collection),
      pm_transactions_active("transactions_active", secs_to_ticks(1), &cache_collection),
      pm_transactions_committing("transactions_committing", secs_to_ticks(1), &cache_collection),
      pm_flushes_diff_flush("flushes_diff_flushing", secs_to_ticks(1), &cache_collection),
      pm_flushes_diff_store("flushes_diff_store", secs_to_ticks(1), &cache_collection),
      pm_flushes_locking("flushes_locking", secs_to_ticks(1), &cache_collection),
      pm_flushes_writing("flushes_writing", secs_to_ticks(1), &cache_collection),
      pm_flushes_blocks("flushes_blocks", secs_to_ticks(1), true, &cache_collection),
      pm_flushes_blocks_dirty("flushes_blocks_need_flush", secs_to_ticks(1), true, &cache_collection),
      pm_flushes_diff_patches_stored("flushes_diff_patches_stored", secs_to_ticks(1), false, &cache_collection),
      pm_flushes_diff_storage_failures("flushes_diff_storage_failures", secs_to_ticks(30), true, &cache_collection),
      pm_n_blocks_in_memory("blocks_in_memory[blocks]", &cache_collection),
      pm_n_blocks_dirty("blocks_dirty[blocks]", &cache_collection),
      pm_n_blocks_total("blocks_total[blocks]", &cache_collection),
      pm_patches_size_ratio("patches_size_ratio", secs_to_ticks(5), false, &cache_collection),
      pm_n_blocks_evicted("blocks_evicted", &cache_collection)
{ }


