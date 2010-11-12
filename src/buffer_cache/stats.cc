#include "buffer_cache/stats.hpp"

perfmon_counter_t
    pm_n_bufs_acquired("bufs_acquired[tbufs]"),
    pm_n_bufs_ready("bufs_ready[tbufs]"),
    pm_n_bufs_released("bufs_released[tbufs]");

perfmon_counter_t
    pm_n_blocks_in_memory("blocks_in_memory[blocks]"),
    pm_n_blocks_dirty("blocks_dirty[blocks]"),
    pm_n_blocks_total("blocks_total[blocks]");

perfmon_counter_t
    pm_n_cows_made("cows_made[cows]"),
    pm_n_cows_destroyed("cows_destroyed[cows]");
