#include "buffer_cache/stats.hpp"

perfmon_counter_t
    pm_n_transactions_started("transactions_started[txns]"),
    pm_n_transactions_ready("transactions_ready[txns]"),
    pm_n_transactions_committed("transactions_committed[txns]"),
    pm_n_transactions_completed("transactions_completed[txns]");

perfmon_counter_t
    pm_n_bufs_acquired("bufs_acquired[tbufs]"),
    pm_n_bufs_ready("bufs_ready[tbufs]"),
    pm_n_bufs_released("bufs_released[tbufs]");

perfmon_counter_t
    pm_n_blocks_in_memory("blocks_in_memory[blocks]"),
    pm_n_blocks_dirty("blocks_dirty[blocks]"),
    pm_n_blocks_total("blocks_total[blocks]");

perfmon_counter_t
    pm_flushes_started("flushes_started[flushes]"),
    pm_flushes_acquired_lock("flushes_acquired_lock[flushes]"),
    pm_flushes_completed("flushes_completed[flushes]");