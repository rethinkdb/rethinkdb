#include "buffer_cache/stats.hpp"

perfmon_counter_t
    pm_n_transactions_started("transactions_started"),
    pm_n_transactions_ready("transactions_ready"),
    pm_n_transactions_committed("transactions_committed"),
    pm_n_transactions_completed("transactions_completed");

perfmon_counter_t
    pm_n_bufs_acquired("bufs_acquired"),
    pm_n_bufs_ready("bufs_ready"),
    pm_n_bufs_released("bufs_released");

perfmon_counter_t
    pm_n_blocks_in_memory("blocks_in_memory"),
    pm_n_blocks_dirty("blocks_dirty"),
    pm_n_blocks_total("blocks_total");

perfmon_counter_t
    pm_flushes_started("flushes_started"),
    pm_flushes_acquired_lock("flushes_acquired_lock"),
    pm_flushes_completed("flushes_completed");